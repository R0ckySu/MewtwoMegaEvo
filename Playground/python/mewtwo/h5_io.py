"""Low-level HDF5 I/O operations for MewtwoMegaEvo data files.

Handles:
- Armadillo complex compound type reading (real + imag fields)
- File enumeration for job-sliced HDF5 outputs
- Parameter list collection from /param_lists/
- HDF5 dataset path reading for meas_marker, meas_all, rho_marker, propagator
"""

import os
import re
from typing import Any, Optional

import h5py
import numpy as np


# =============================================================================
# Complex Number Handling
# =============================================================================


def read_complex_dataset(h5file: h5py.File, path: str) -> np.ndarray:
    """Read an HDF5 dataset, handling Armadillo complex compound type.

    Armadillo stores complex numbers as a compound HDF5 type with 'real'
    and 'imag' fields. This function detects that format and reconstructs
    a proper complex-valued numpy array.

    Parameters
    ----------
    h5file : h5py.File
        Open HDF5 file handle.
    path : str
        Full HDF5 path to the dataset (e.g. '/rho_marker/#0/Z').

    Returns
    -------
    np.ndarray
        The dataset as a numpy array. Complex compound types are
        combined into complex128; real datasets are returned as-is.
    """
    ds = h5file[path]
    data = ds[()]

    # Armadillo complex compound type: structured array with 'real'/'imag' columns
    if data.dtype.names is not None:
        names = data.dtype.names
        if "real" in names and "imag" in names:
            return data["real"] + 1j * data["imag"]

    return data


# =============================================================================
# File Enumeration
# =============================================================================


def get_file_list_with_pattern(
    folder_path: str, task_name: str, pattern: str = r"[0-9]+_Job#[0-9]{1,3}$"
) -> list[str]:
    """Find job-sliced HDF5 files matching a naming pattern.

    Port of GetFileNameListWithPattern.m.

    Files are expected to be named like: <task_name><timestamp>_Job#<N>
    They are sorted by their Job# number.

    Parameters
    ----------
    folder_path : str
        Directory containing HDF5 files.
    task_name : str
        Task name prefix to match.
    pattern : str
        Regex pattern for the job-sliced file suffix.
        Default: r"[0-9]+_Job#[0-9]{1,3}$"

    Returns
    -------
    list[str]
        Sorted list of matching file names (basenames only).
    """
    full_pattern = re.compile(task_name + pattern)

    matching_files = [
        f for f in os.listdir(folder_path) if full_pattern.search(f)
    ]

    # Sort by extracted Job# index
    def _extract_job_id(filename: str) -> int:
        m = re.search(r"Job#(\d+)", filename)
        return int(m.group(1)) if m else 0

    matching_files.sort(key=_extract_job_id)
    return matching_files


# =============================================================================
# Parameter List Collection
# =============================================================================


def collect_param_lists(
    data_path: str, file_names: list[str]
) -> dict[str, Any]:
    """Collect and concatenate parameter lists from job-sliced HDF5 files.

    Port of CollecrH5ParamList.m.

    Reads /param_lists/ from each HDF5 file, concatenating parameter
    vectors across job slices (since each job handles a contiguous
    chunk of the flattened parameter space).

    Parameters
    ----------
    data_path : str
        Directory containing the HDF5 files.
    file_names : list[str]
        Basenames of the HDF5 files (from get_file_list_with_pattern).

    Returns
    -------
    dict
        Keys:
        - param_names : list[str] — dataset names from /param_lists/
        - num_of_params_for_each_file : np.ndarray — count per file
        - <param_name> : np.ndarray — concatenated 1-D parameter vector
          for each parameter (key is the name without '#' prefix if present)
    """
    result: dict[str, Any] = {}
    param_names: Optional[list[str]] = None
    num_of_params_for_each_file = np.zeros(len(file_names), dtype=int)

    for f_idx, file_name in enumerate(file_names):
        file_path = os.path.join(data_path, file_name)
        with h5py.File(file_path, "r") as f:
            param_group = f["/param_lists"]
            names = list(param_group.keys())
            num_of_params_for_each_file[f_idx] = len(param_group[names[0]])

            for name in names:
                # Strip leading '#' from param names if present
                clean_name = name.lstrip("#")
                loaded = param_group[name][:]

                if f_idx == 0:
                    result[clean_name] = loaded
                else:
                    result[clean_name] = np.concatenate(
                        [result[clean_name], loaded]
                    )

            if param_names is None:
                param_names = names

    result["param_names"] = param_names or []
    result["num_of_params_for_each_file"] = num_of_params_for_each_file
    return result


# =============================================================================
# HDF5 Dataset Readers
# =============================================================================


def read_meas_marker(
    h5file: h5py.File, param_idx: int, obs: str, init_state: str
) -> np.ndarray:
    """Read a single meas_marker dataset.

    Path: /meas_marker/#<param_idx>/<obs>/<init_state>
    """
    path = f"/meas_marker/#{param_idx}/{obs}/{init_state}"
    return h5file[path][:]


def read_meas_marker_with_time(
    h5file: h5py.File, param_idx: int, obs: str, init_state: str
) -> tuple[np.ndarray, np.ndarray]:
    """Read meas_marker data and its time vector.

    Returns
    -------
    tuple[np.ndarray, np.ndarray]
        (time_vec, meas_data)
    """
    group_path = f"/meas_marker/#{param_idx}"
    time_vec = h5file[f"{group_path}/time_vec"][:]
    meas_data = h5file[f"{group_path}/{obs}/{init_state}"][:]
    return time_vec, meas_data


def read_meas_all(
    h5file: h5py.File, param_idx: int, obs: str, init_state: str
) -> np.ndarray:
    """Read a single meas_all dataset (full time trace).

    Path: /meas_all/#<param_idx>/<obs>/<init_state>
    """
    path = f"/meas_all/#{param_idx}/{obs}/{init_state}"
    return h5file[path][:]


def read_meas_all_with_time(
    h5file: h5py.File, param_idx: int, obs: str, init_state: str
) -> tuple[np.ndarray, np.ndarray]:
    """Read meas_all data and its time vector."""
    group_path = f"/meas_all/#{param_idx}"
    time_vec = h5file[f"{group_path}/time_vec"][:]
    meas_data = h5file[f"{group_path}/{obs}/{init_state}"][:]
    return time_vec, meas_data


def read_rho_marker(
    h5file: h5py.File, param_idx: int, init_state: str
) -> np.ndarray:
    """Read a single rho_marker density matrix (complex compound type).

    Path: /rho_marker/#<param_idx>/<init_state>
    """
    path = f"/rho_marker/#{param_idx}/{init_state}"
    return read_complex_dataset(h5file, path)


def read_propagator(
    h5file: h5py.File, param_idx: int
) -> np.ndarray:
    """Read a single propagator (complex compound type).

    Path: /propagator/#<param_idx>
    """
    path = f"/propagator/#{param_idx}"
    return read_complex_dataset(h5file, path)


# =============================================================================
# HDF5 Group Info (for ParamFold-based access)
# =============================================================================


def get_param_name_to_file_lookup(
    data_path: str, file_names: list[str]
) -> dict[str, str]:
    """Build a lookup table mapping HDF5 group paths to their containing file.

    Port of GetParamName2JobIdLookupTable.m.

    For job-sliced data accessed via ParamFold field names (e.g.
    '/#mw_freq=1.0e9#mw_freq=1.0e9'), this maps each group path
    to the HDF5 file that contains it.

    Parameters
    ----------
    data_path : str
        Directory containing HDF5 files.
    file_names : list[str]
        Basenames of HDF5 files.

    Returns
    -------
    dict[str, str]
        Mapping: {hdf5_group_path: file_basename}
    """
    lookup: dict[str, str] = {}
    for file_name in file_names:
        file_path = os.path.join(data_path, file_name)
        with h5py.File(file_path, "r") as f:
            # Iterate top-level groups (each is a param field name)
            for group_name in f.keys():
                if group_name.startswith("#"):
                    lookup[f"/{group_name}"] = file_name
    return lookup
