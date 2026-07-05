"""Main data loaders for MewtwoMegaEvo simulation output.

Port of LoadMewtwoData.m and LoadMeasMarkerData.m.

Provides:
- load_mewtwo_data() — Primary entry point. Reads HDF5 files from job-sliced
  simulation output, reconstructs N-dimensional parameter grids via ParamFold,
  and returns an xarray Dataset. Caches results as NetCDF.
- load_meas_marker_data() — Alternative loader using ParamFold-based
  field-name HDF5 access (matching LoadMeasMarkerData.m).
"""

import os
import warnings
from typing import Any, Optional

import h5py
import numpy as np
import xarray as xr

from mewtwo.config import load_all_configs
from mewtwo.h5_io import (
    collect_param_lists,
    get_file_list_with_pattern,
    read_complex_dataset,
)
from mewtwo.params import param_fold
from mewtwo.xarray_builder import build_dataset


# =============================================================================
# Cache Helpers
# =============================================================================


def _cache_path(data_path: str, task_name: str) -> str:
    """Get the NetCDF cache file path."""
    return os.path.join(data_path, f"{task_name}_organised.nc")


def _load_cached(data_path: str, task_name: str) -> Optional[xr.Dataset]:
    """Try to load a cached NetCDF dataset."""
    path = _cache_path(data_path, task_name)
    if os.path.isfile(path):
        return xr.open_dataset(path)
    return None


def _save_cache(ds: xr.Dataset, data_path: str, task_name: str) -> None:
    """Save dataset as NetCDF cache.

    Uses auto_complex=True to handle complex dtypes (splits into
    real + imag components, as NetCDF4 doesn't natively support
    complex numbers).
    """
    path = _cache_path(data_path, task_name)
    ds.to_netcdf(path, auto_complex=True)
    print(f"Cached: {path}")


# =============================================================================
# Main Loader
# =============================================================================


def load_mewtwo_data(
    data_path: str,
    force_reload: bool = False,
) -> xr.Dataset:
    """Load MewtwoMegaEvo simulation results as an xarray Dataset.

    Primary entry point for loading simulation output. Reads all HDF5
    files in a result directory (including job-sliced files), folds
    flattened parameter sweeps back into N-dimensional arrays, and
    returns a labeled xarray Dataset.

    Caches the result as a NetCDF file (``<task_name>_organised.nc``)
    for fast subsequent loading.

    Parameters
    ----------
    data_path : str
        Path to the simulation result directory (e.g.,
        ``Playground/sim_results/RabiChevron20260614131343/``).
        Must contain a ``config_files/`` subdirectory with the JSON
        configuration files, and one or more HDF5 output files.
    force_reload : bool
        If True, skip the NetCDF cache and re-read from HDF5.

    Returns
    -------
    xr.Dataset
        Labeled dataset with:
        - Dimensions named after swept parameters
        - Coordinates holding unique parameter values
        - Data variables: ``meas_marker_<obs>_<init>`` for each
          observable/initial-state pair
        - Additional variables if ``record_all_meas``,
          ``record_density_mat``, or ``record_propagator`` were enabled:
          ``meas_all_<obs>_<init>``, ``rho_marker_<init>``, ``propagator``

    Examples
    --------
    >>> import mewtwo
    >>> ds = mewtwo.load_mewtwo_data(
    ...     "Playground/sim_results/RabiChevron20260614131343"
    ... )
    >>> print(ds)
    """
    # Extract task name from path (last component)
    data_path = os.path.normpath(data_path)
    task_name = os.path.basename(data_path)

    # Check cache
    if not force_reload:
        cached = _load_cached(data_path, task_name)
        if cached is not None:
            print(f"Loaded from cache: {_cache_path(data_path, task_name)}")
            return cached

    # Load configuration files
    config_dir = os.path.join(data_path, "config_files")
    if not os.path.isdir(config_dir):
        raise FileNotFoundError(
            f"Config directory not found: {config_dir}. "
            f"Make sure the data_path points to a valid simulation result folder."
        )

    sim_config, gate_config, hamiltonian_config = load_all_configs(data_path)
    task_name_from_config = sim_config.get("task_name", task_name)

    # Find HDF5 files
    h5_files = get_file_list_with_pattern(
        data_path, task_name_from_config
    )
    if not h5_files:
        raise FileNotFoundError(
            f"No HDF5 files found in {data_path} matching "
            f"'{task_name_from_config}*_Job#<N>'"
        )
    print(f"Found {len(h5_files)} job-sliced HDF5 file(s)")

    # Collect parameter lists from all files
    print("Collecting parameter lists...")
    param_info = collect_param_lists(data_path, h5_files)

    # Compute N-D grid info via ParamFold
    sweep_param_info = sim_config.get("sweep_param_info", [])
    if sweep_param_info:
        fold_info = param_fold(config_dir, sweep_param_info)
        total_points = fold_info["total_points"]
    else:
        # No parametric sweep — single-point simulation
        fold_info = {
            "param_space_dims": [],
            "param_keys_ordered": [],
            "axis_coords": {},
            "axis_labels": {},
            "field_names": [],
            "field_name_to_index": {},
            "unique_params": {},
            "total_points": 1,
        }
        total_points = 1

    # Configuration flags
    observables = sim_config.get("observables", [])
    init_states = sim_config.get("init_states", [])
    record_all_meas = sim_config.get("record_all_meas", False)
    record_density_mat = sim_config.get("record_density_mat", False)
    record_propagator = sim_config.get("record_propagator", False)

    # Initialize flat collection containers
    meas_marker_flat: dict[str, dict[str, list[np.ndarray]]] = {
        obs: {init: [] for init in init_states} for obs in observables
    }
    meas_all_flat: Optional[dict[str, dict[str, list[np.ndarray]]]] = (
        {obs: {init: [] for init in init_states} for obs in observables}
        if record_all_meas
        else None
    )
    rho_marker_flat: Optional[dict[str, list[np.ndarray]]] = (
        {init: [] for init in init_states} if record_density_mat else None
    )
    propagator_flat: Optional[list[np.ndarray]] = (
        [] if record_propagator else None
    )

    # Read data from HDF5 files
    print("Reading HDF5 data...")
    params_per_file = param_info["num_of_params_for_each_file"]

    for file_name in h5_files:
        file_path = os.path.join(data_path, file_name)
        with h5py.File(file_path, "r") as f:
            n_params = _count_params_in_file(f)

            for p_idx in range(n_params):
                # Read meas_marker
                for obs in observables:
                    for init in init_states:
                        try:
                            data = _safe_read_dataset(
                                f, f"/meas_marker/#{p_idx}/{obs}/{init}"
                            )
                            meas_marker_flat[obs][init].append(data)
                        except KeyError:
                            warnings.warn(
                                f"Dataset not found: /meas_marker/#{p_idx}/{obs}/{init}"
                            )

                # Read meas_all (full time traces)
                if record_all_meas and meas_all_flat is not None:
                    for obs in observables:
                        for init in init_states:
                            try:
                                data = _safe_read_dataset(
                                    f, f"/meas_all/#{p_idx}/{obs}/{init}"
                                )
                                meas_all_flat[obs][init].append(data)
                            except KeyError:
                                pass

                # Read rho_marker (density matrices)
                if record_density_mat and rho_marker_flat is not None:
                    for init in init_states:
                        try:
                            data = read_complex_dataset(
                                f, f"/rho_marker/#{p_idx}/{init}"
                            )
                            rho_marker_flat[init].append(data)
                        except KeyError:
                            pass

                # Read propagator
                if record_propagator and propagator_flat is not None:
                    try:
                        data = read_complex_dataset(
                            f, f"/propagator/#{p_idx}"
                        )
                        propagator_flat.append(data)
                    except KeyError:
                        pass

    # Build xarray Dataset
    print("Building xarray Dataset...")
    ds = build_dataset(
        task_name=task_name_from_config,
        data_path=data_path,
        config_info=sim_config,
        gate_config=gate_config,
        hamiltonian_config=hamiltonian_config,
        fold_info=fold_info,
        meas_marker_flat=meas_marker_flat,
        meas_all_flat=meas_all_flat,
        rho_marker_flat=rho_marker_flat,
        propagator_flat=propagator_flat,
    )

    # Cache as NetCDF
    _save_cache(ds, data_path, task_name)

    return ds


# =============================================================================
# Alternative Loader: ParamFold Field-Name Access
# =============================================================================


def load_meas_marker_data(
    data_path: str,
    force_reload: bool = False,
) -> xr.Dataset:
    """Load measurement marker data using ParamFold field-name HDF5 access.

    Alternative loader matching LoadMeasMarkerData.m. Uses ParamFold to
    construct HDF5 path names (e.g., ``/#mw_freq=1.0e9/Z/Z``) for
    direct per-grid-point access. This provides finer control over the
    N-D data reconstruction compared to the linear-index approach in
    load_mewtwo_data().

    Parameters
    ----------
    data_path : str
        Path to the simulation result directory.
    force_reload : bool
        If True, skip NetCDF cache.

    Returns
    -------
    xr.Dataset
    """
    data_path = os.path.normpath(data_path)
    task_name = os.path.basename(data_path)

    # Check cache
    if not force_reload:
        cached = _load_cached(data_path, task_name)
        if cached is not None:
            return cached

    sim_config, gate_config, hamiltonian_config = load_all_configs(data_path)
    config_dir = os.path.join(data_path, "config_files")

    sweep_param_info = sim_config.get("sweep_param_info", [])
    if not sweep_param_info:
        raise ValueError("No sweep_param_info in sim_config.json")

    fold_info = param_fold(config_dir, sweep_param_info)
    observables = sim_config.get("observables", [])
    init_states = sim_config.get("init_states", [])

    h5_files = get_file_list_with_pattern(
        data_path, sim_config.get("task_name", task_name)
    )

    # Build field-name to file lookup
    from mewtwo.h5_io import get_param_name_to_file_lookup

    field_to_file = get_param_name_to_file_lookup(data_path, h5_files)

    # Read data using field names
    meas_marker_flat: dict[str, dict[str, list[np.ndarray]]] = {
        obs: {init: [] for init in init_states} for obs in observables
    }

    for field_name in fold_info["field_names"]:
        h5_path = f"/{field_name}"
        file_name = field_to_file.get(h5_path)
        if file_name is None:
            warnings.warn(f"Field name not found in any file: {h5_path}")
            # Append placeholder
            for obs in observables:
                for init in init_states:
                    meas_marker_flat[obs][init].append(np.array([]))
            continue

        file_path = os.path.join(data_path, file_name)
        with h5py.File(file_path, "r") as f:
            for obs in observables:
                for init in init_states:
                    try:
                        data = _safe_read_dataset(
                            f, f"{h5_path}/{obs}/{init}"
                        )
                        meas_marker_flat[obs][init].append(data)
                    except KeyError:
                        meas_marker_flat[obs][init].append(np.array([]))

    ds = build_dataset(
        task_name=sim_config.get("task_name", task_name),
        data_path=data_path,
        config_info=sim_config,
        gate_config=gate_config,
        hamiltonian_config=hamiltonian_config,
        fold_info=fold_info,
        meas_marker_flat=meas_marker_flat,
    )

    _save_cache(ds, data_path, task_name)
    return ds


# =============================================================================
# Helpers
# =============================================================================


def _count_params_in_file(h5file: h5py.File) -> int:
    """Count the number of parameter points in an HDF5 file.

    Counts groups under /meas_marker/ matching the #<N> pattern.
    """
    if "/meas_marker" not in h5file:
        return 0
    group = h5file["/meas_marker"]
    count = 0
    for name in group.keys():
        if name.startswith("#"):
            try:
                int(name[1:])
                count += 1
            except ValueError:
                pass
    return count


def _safe_read_dataset(h5file: h5py.File, path: str) -> np.ndarray:
    """Read an HDF5 dataset, returning empty array on KeyError."""
    return h5file[path][:]
