"""Build xarray Dataset from flat loaded HDF5 data.

Reconstructs N-dimensional arrays from the flattened parameter sweep
results and packages them as labeled xarray Datasets with proper
dimension coordinates and metadata.
"""

import json as _json
import re
from typing import Any, Optional

import numpy as np
import xarray as xr


# =============================================================================
# Dimension Name Helpers
# =============================================================================


def _sanitize_dim_name(name: str) -> str:
    """Convert a parameter key to a valid xarray dimension name."""
    sanitized = re.sub(r"[^a-zA-Z0-9_]", "_", name)
    if sanitized and sanitized[0].isdigit():
        sanitized = "dim_" + sanitized
    return sanitized


def _make_dim_names_unique(names: list[str]) -> list[str]:
    """Ensure dimension names are unique by appending suffixes."""
    seen: dict[str, int] = {}
    result = []
    for name in names:
        sanitized = _sanitize_dim_name(name)
        if sanitized in seen:
            seen[sanitized] += 1
            result.append(f"{sanitized}_{seen[sanitized]}")
        else:
            seen[sanitized] = 0
            result.append(sanitized)
    return result


def _infer_data_dim_names(
    base: str, data_shape: tuple[int, ...]
) -> list[str]:
    """Generate meaningful shared dimension names for per-point data axes.

    Uses heuristics based on the base name (meas_marker, rho_marker, etc.)
    and axis position to create reusable dimension names.

    Parameters
    ----------
    base : str
        Category name like 'meas_marker', 'rho_marker', 'meas_all', 'propagator'.
    data_shape : tuple
        Shape of the per-point data (e.g., (1, 80) for meas_marker with
        1 repeat and 80 measurement markers).

    Returns
    -------
    list[str]
        Dimension names of length len(data_shape).
    """
    if base == "meas_marker":
        if len(data_shape) == 2:
            return ["marker_repeat", "marker_index"]
        elif len(data_shape) == 1:
            return ["marker_index"]
        return [f"marker_dim{i}" for i in range(len(data_shape))]

    elif base == "meas_all":
        if len(data_shape) == 2:
            return ["trace_repeat", "time"]
        elif len(data_shape) == 1:
            return ["time"]
        names = [f"trace_dim{i}" for i in range(len(data_shape) - 1)]
        names.append("time")
        return names

    elif base == "rho_marker":
        if len(data_shape) == 3:
            return ["dm_repeat", "dm_row", "dm_col"]
        elif len(data_shape) == 2:
            return ["dm_row", "dm_col"]
        return [f"dm_dim{i}" for i in range(len(data_shape))]

    elif base == "propagator":
        if len(data_shape) == 2:
            return ["prop_row", "prop_col"]
        return [f"prop_dim{i}" for i in range(len(data_shape))]

    else:
        return [f"{base}_dim{i}" for i in range(len(data_shape))]


def _get_common_data_shape(
    flat_dict: dict[str, dict[str, list[np.ndarray]]],
) -> Optional[tuple[int, ...]]:
    """Find the common per-point data shape across all obs/init combos."""
    for obs_data in flat_dict.values():
        for flat_list in obs_data.values():
            if flat_list:
                return flat_list[0].shape
    return None


def _get_flat_list_shape(
    flat_list_dict: Optional[dict[str, list[np.ndarray]]],
) -> Optional[tuple[int, ...]]:
    """Find the common per-point data shape from {init: list} dict."""
    if flat_list_dict is None:
        return None
    for flat_list in flat_list_dict.values():
        if flat_list:
            return flat_list[0].shape
    return None


# =============================================================================
# Data Folding
# =============================================================================


def _fold_nd_data(
    data_stack: np.ndarray,
    grid_dims: list[int],
    dim_names: list[str],
    coords: dict[str, np.ndarray],
) -> tuple[np.ndarray, list[str], dict[str, np.ndarray]]:
    """Fold the first axis of data_stack into N-D grid dimensions.

    Parameters
    ----------
    data_stack : np.ndarray
        Shape (total_points, *data_shape). The first axis is the flat
        parameter index; remaining axes are per-point data structure.
    grid_dims : list[int]
        N-D grid dimensions [N1, N2, ..., Nk] whose product = total_points.
    dim_names : list[str]
        Dimension names for each grid axis.
    coords : dict[str, np.ndarray]
        Coordinate arrays for grid axes.

    Returns
    -------
    tuple
        (folded_data, all_dim_names, all_coords)
        folded_data has shape (*grid_dims, *data_shape).
    """
    data_shape = data_stack.shape[1:]

    if len(grid_dims) == 0:
        folded = data_stack[0].copy()
        return folded, [], {}

    elif len(grid_dims) == 1:
        return data_stack, dim_names, coords

    else:
        # N-D sweep: reshape flat param axis into grid using Fortran ordering
        # (flat order: param1 varies fastest, paramN varies slowest)
        flat_2d = data_stack.reshape(data_stack.shape[0], -1)
        folded_2d = flat_2d.reshape(*grid_dims, -1, order="F")
        folded = folded_2d.reshape(*grid_dims, *data_shape)
        return folded, dim_names, coords


# =============================================================================
# Main Builder
# =============================================================================


def build_dataset(
    task_name: str,
    data_path: str,
    config_info: dict[str, Any],
    gate_config: dict[str, Any],
    hamiltonian_config: dict[str, Any],
    fold_info: dict[str, Any],
    meas_marker_flat: dict[str, dict[str, list[np.ndarray]]],
    meas_all_flat: Optional[dict[str, dict[str, list[np.ndarray]]]] = None,
    rho_marker_flat: Optional[dict[str, list[np.ndarray]]] = None,
    propagator_flat: Optional[list[np.ndarray]] = None,
) -> xr.Dataset:
    """Build an xarray Dataset from flat-loaded simulation data.

    Parameters
    ----------
    task_name : str
        Name of the simulation task.
    data_path : str
        Path to the simulation result directory.
    config_info : dict
        Parsed sim_config.json.
    gate_config : dict
        Parsed gate_config.json.
    hamiltonian_config : dict
        Parsed hamiltonian_config.json.
    fold_info : dict
        Output of param_fold().
    meas_marker_flat : dict
        Nested: {obs_name: {init_state: [array_at_point_0, ...]}}.
    meas_all_flat : dict, optional
        Same structure for full time traces.
    rho_marker_flat : dict, optional
        {init_state: [matrix_at_point_0, ...]}.
    propagator_flat : list, optional
        [propagator_at_point_0, ...].

    Returns
    -------
    xr.Dataset
    """
    grid_dims = fold_info["param_space_dims"]
    param_keys = fold_info["param_keys_ordered"]
    axis_coords = fold_info["axis_coords"]
    axis_labels = fold_info["axis_labels"]

    dim_names = _make_dim_names_unique(param_keys)

    data_vars: dict[str, xr.DataArray] = {}
    coords: dict[str, np.ndarray] = {}

    for dim_name, key in zip(dim_names, param_keys):
        coords[dim_name] = axis_coords[key]

    observables = config_info.get("observables", [])
    init_states = config_info.get("init_states", [])

    # =========================================================================
    # Meas Marker Data — determine shared inner dims once
    # =========================================================================
    mm_data_shape = _get_common_data_shape(meas_marker_flat)
    mm_inner_dims = _infer_data_dim_names("meas_marker", mm_data_shape) if mm_data_shape else []

    for obs in observables:
        for init in init_states:
            flat_list = meas_marker_flat.get(obs, {}).get(init, [])
            if not flat_list:
                continue

            var_name = f"meas_marker_{obs}_{init}"

            if _can_stack_to_ndarray(flat_list):
                data_stack = np.array(flat_list)
                nd_data, all_dims, all_coords = _fold_nd_data(
                    data_stack, grid_dims, dim_names, coords
                )
                da = xr.DataArray(
                    nd_data,
                    dims=[*all_dims, *mm_inner_dims],
                    coords=all_coords,
                    name=var_name,
                )
            else:
                data_nd = _reshape_ragged(flat_list, grid_dims)
                da = xr.DataArray(
                    data_nd,
                    dims=dim_names,
                    coords=coords,
                    name=var_name,
                )
            data_vars[var_name] = da

    # =========================================================================
    # Meas All (Full Time Traces) — determine shared inner dims once
    # =========================================================================
    if meas_all_flat is not None:
        ma_data_shape = _get_common_data_shape(meas_all_flat)
        ma_inner_dims = (
            _infer_data_dim_names("meas_all", ma_data_shape)
            if ma_data_shape
            else []
        )
        step_size = config_info.get("step_size", 1e-9)

        for obs in observables:
            for init in init_states:
                flat_list = meas_all_flat.get(obs, {}).get(init, [])
                if not flat_list:
                    continue

                var_name = f"meas_all_{obs}_{init}"
                if _can_stack_to_ndarray(flat_list):
                    data_stack = np.array(flat_list)
                    nd_data, all_dims, all_coords = _fold_nd_data(
                        data_stack, grid_dims, dim_names, coords
                    )

                    # Add time coordinate if "time" is the last inner dim
                    if ma_inner_dims and ma_inner_dims[-1] == "time":
                        all_coords = dict(all_coords)
                        num_time = data_stack.shape[-1]
                        all_coords["time"] = np.arange(num_time) * step_size

                    da = xr.DataArray(
                        nd_data,
                        dims=[*all_dims, *ma_inner_dims],
                        coords=all_coords,
                        name=var_name,
                    )
                    data_vars[var_name] = da

    # =========================================================================
    # Rho Marker (Density Matrices) — determine shared inner dims once
    # =========================================================================
    if rho_marker_flat is not None:
        dm_data_shape = _get_flat_list_shape(rho_marker_flat)
        dm_inner_dims = (
            _infer_data_dim_names("rho_marker", dm_data_shape)
            if dm_data_shape
            else []
        )

        for init in init_states:
            flat_list = rho_marker_flat.get(init, [])
            if not flat_list:
                continue

            var_name = f"rho_marker_{init}"
            if _can_stack_to_ndarray(flat_list):
                data_stack = np.array(flat_list)
                nd_data, all_dims, all_coords = _fold_nd_data(
                    data_stack, grid_dims, dim_names, coords
                )
                da = xr.DataArray(
                    nd_data,
                    dims=[*all_dims, *dm_inner_dims],
                    coords=all_coords,
                    name=var_name,
                )
                data_vars[var_name] = da

    # =========================================================================
    # Propagator — determine inner dims
    # =========================================================================
    if propagator_flat is not None and len(propagator_flat) > 0:
        if _can_stack_to_ndarray(propagator_flat):
            data_stack = np.array(propagator_flat)
            prop_data_shape = data_stack.shape[1:]
            prop_inner_dims = _infer_data_dim_names("propagator", prop_data_shape)

            nd_data, all_dims, all_coords = _fold_nd_data(
                data_stack, grid_dims, dim_names, coords
            )
            da = xr.DataArray(
                nd_data,
                dims=[*all_dims, *prop_inner_dims],
                coords=all_coords,
                name="propagator",
            )
            data_vars["propagator"] = da

    # =========================================================================
    # Assemble Dataset
    # =========================================================================
    ds = xr.Dataset(data_vars)

    ds.attrs["task_name"] = task_name
    ds.attrs["data_path"] = data_path
    ds.attrs["dim_labels"] = _json.dumps(
        {dim: axis_labels.get(key, key) for dim, key in zip(dim_names, param_keys)}
    )
    ds.attrs["config_info"] = _json.dumps(config_info)
    ds.attrs["gate_config"] = _json.dumps(gate_config)
    ds.attrs["hamiltonian_config"] = _json.dumps(hamiltonian_config)

    return ds


# =============================================================================
# Helpers
# =============================================================================


def _can_stack_to_ndarray(flat_list: list[np.ndarray]) -> bool:
    """Check if all arrays in the list have the same shape."""
    if not flat_list:
        return False
    first_shape = flat_list[0].shape
    return all(arr.shape == first_shape for arr in flat_list)


def _reshape_ragged(
    flat_list: list[np.ndarray], grid_dims: list[int]
) -> np.ndarray:
    """Reshape a ragged list into an N-D object array."""
    if len(grid_dims) <= 1:
        return np.array(flat_list, dtype=object)

    result = np.empty(grid_dims, dtype=object)
    for i, arr in enumerate(flat_list):
        idx = np.unravel_index(i, grid_dims, order="F")
        result[idx] = arr
    return result
