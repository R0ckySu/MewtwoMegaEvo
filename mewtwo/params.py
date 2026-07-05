"""Multi-dimensional parameter folding and spanning.

Port of ParamFold.m and ParamSpan.m.

ParamSpan: Takes N 1-D parameter lists, meshgrids them into an N-D space,
and flattens the result into 1-D "span" vectors that Mewtwo can iterate over.

ParamFold: The inverse operation — reads the spanned parameter vectors,
extracts unique values along each axis, determines grid dimensions, and
provides the information needed to fold flat results back into N-D arrays.
"""

import os
from typing import Any, Union

import numpy as np


def _read_param_file(file_path: str) -> np.ndarray:
    """Read a parameter file (CSV numeric or newline-separated string list).

    Returns a 1-D numpy array (float if numeric, string/object otherwise).
    """
    with open(file_path, "r") as f:
        content = f.read()

    lines = [line.strip() for line in content.split("\n") if line.strip()]

    if not lines:
        raise ValueError(f"Parameter file is empty: {file_path}")

    # Try to parse as numeric
    try:
        return np.array([float(line) for line in lines])
    except ValueError:
        # String parameters (e.g., sequence aliases)
        return np.array(lines)


def _unique_stable(arr: np.ndarray) -> np.ndarray:
    """Return unique values preserving first-occurrence order.

    Equivalent to MATLAB's unique(x, 'stable').
    """
    _, idx = np.unique(arr, return_index=True)
    return arr[np.sort(idx)]


def param_fold(
    config_dir: str,
    sweep_param_info: list[dict[str, Any]],
) -> dict[str, Any]:
    """Analyze spanned parameter files to determine N-D grid structure.

    Port of ParamFold.m.

    Given the sweep_param_info from sim_config.json and the config_files
    directory, this reads each spanned parameter vector, extracts unique
    values, and computes the N-D grid dimensions, axis coordinates, and
    labels needed to fold flat simulation results back into multi-
    dimensional arrays.

    Parameters
    ----------
    config_dir : str
        Path to the config_files directory containing parameter data files.
    sweep_param_info : list[dict]
        The sweep_param_info array from sim_config.json. Each entry has:
        - class : str — object class (Hamiltonian, Gate, Sequence)
        - tag : str — object identifier
        - property : str — property being swept
        - val_file : str (optional) — file containing numeric param values
        - string_file : str (optional) — file containing string param values

    Returns
    -------
    dict
        Keys:
        - param_space_dims : list[int] — grid dimensions [N1, N2, ..., Nk]
        - param_keys_ordered : list[str] — dimension names in order
        - axis_coords : dict[str, np.ndarray] — unique coordinate values per axis
        - axis_labels : dict[str, str] — human-readable axis labels
        - field_names : list[str] — HDF5 field name strings for each grid point
        - field_name_to_index : dict[str, np.ndarray] — field_name → [i1,i2,...] indices
        - unique_params : dict[str, np.ndarray] — unique values per parameter
    """
    series_of_params = len(sweep_param_info)

    # Containers
    spanned_params: dict[str, np.ndarray] = {}
    unique_param_dic: dict[str, np.ndarray] = {}
    axis_coords: dict[str, np.ndarray] = {}
    axis_labels: dict[str, str] = {}
    param_keys_ordered: list[str] = []

    param_space_dims: list[int] = []
    total_num_params = 0

    for param_info in sweep_param_info:
        if "val_file" in param_info:
            file_name = param_info["val_file"]
            param_keys_ordered.append(file_name)
            file_path = os.path.join(config_dir, file_name)
            param_vec = _read_param_file(file_path)

            spanned_params[file_name] = param_vec
            unique_vals = _unique_stable(param_vec)
            # Strip _span suffix for the unique params key (matching MATLAB)
            clean_key = file_name.replace("_span", "")
            unique_param_dic[clean_key] = unique_vals
            param_space_dims.append(len(unique_vals))
            axis_coords[file_name] = unique_vals

        elif "string_file" in param_info:
            file_name = param_info["string_file"]
            param_keys_ordered.append(file_name)
            file_path = os.path.join(config_dir, file_name)
            param_vec = _read_param_file(file_path)

            spanned_params[file_name] = param_vec
            unique_vals = _unique_stable(param_vec)

            # For numeric params, use the values directly as coords.
            # For string params, try to parse as floats; if that fails,
            # keep the actual string values as coordinates (xarray can
            # handle object/string arrays).
            if np.issubdtype(unique_vals.dtype, np.number):
                axis_coords[file_name] = unique_vals
            else:
                try:
                    numeric_vals = np.array([float(v) for v in unique_vals])
                    axis_coords[file_name] = numeric_vals
                except (ValueError, TypeError):
                    # Keep string values as-is for coordinate labels
                    axis_coords[file_name] = unique_vals

            clean_key = file_name.replace("_span", "")
            unique_param_dic[clean_key] = unique_vals
            param_space_dims.append(len(unique_vals))

        # Build axis label
        label = ".".join(
            [param_info.get("class", ""), param_info.get("tag", ""), param_info.get("property", "")]
        )
        axis_labels[file_name] = label

        if total_num_params == 0:
            total_num_params = len(param_vec)
        elif total_num_params != len(param_vec):
            raise ValueError(
                f"Spanned parameter vectors have inconsistent lengths: "
                f"expected {total_num_params}, got {len(param_vec)} for {file_name}"
            )

    # Build HDF5 field name strings and position indices
    field_names: list[str] = []
    field_name_to_index: dict[str, np.ndarray] = {}

    for i in range(total_num_params):
        field_parts: list[str] = []
        pos = np.zeros(series_of_params, dtype=int)
        relative_location = i

        for j in range(series_of_params):
            key = param_keys_ordered[j]
            param_list = spanned_params[key]

            # Format parameter value for field name
            if np.issubdtype(param_list.dtype, np.number):
                val_str = f"{param_list[i]:10.6e}"
            else:
                val_str = str(param_list[i])

            field_parts.append(f"#{key}={val_str}")

            # Compute N-D position index
            if j < series_of_params - 1:
                stride = int(np.prod(param_space_dims[j + 1 :]))
                pos[j] = relative_location // stride
                relative_location = relative_location % stride
            else:
                pos[j] = relative_location

        field_name = "".join(field_parts)
        field_names.append(field_name)
        # Convert to 1-based indices matching MATLAB convention
        field_name_to_index[field_name] = pos + 1

    return {
        "param_space_dims": param_space_dims,
        "param_keys_ordered": param_keys_ordered,
        "axis_coords": axis_coords,
        "axis_labels": axis_labels,
        "field_names": field_names,
        "field_name_to_index": field_name_to_index,
        "unique_params": unique_param_dic,
        "total_points": total_num_params,
    }


def param_span(
    config_dir: str, param_names: list[str]
) -> dict[str, np.ndarray]:
    """Span multiple 1-D parameter lists into flattened N-D grid vectors.

    Port of ParamSpan.m.

    Given N parameter files (each containing a 1-D list of values), this
    performs an N-dimensional meshgrid and flattens the result into N
    vectors of length prod(dims). The flattened vectors are written as
    ``<name>_span`` files in the config directory.

    The flattening order is consistent with Mewtwo's parameter sweep:
    the FIRST parameter varies FASTEST (inner loop), and the LAST
    parameter varies SLOWEST (outer loop). This is equivalent to
    Fortran/column-major ordering.

    Parameters
    ----------
    config_dir : str
        Path to the config_files directory.
    param_names : list[str]
        List of parameter file basenames (without path).

    Returns
    -------
    dict[str, np.ndarray]
        Mapping: {param_name: flattened_1D_array}
        Also includes 'param_space_dims' and 'param_names_ordered'.
    """
    num_params = len(param_names)

    # Read all parameter vectors
    param_vectors: list[np.ndarray] = []
    for name in param_names:
        file_path = os.path.join(config_dir, name)
        param_vectors.append(_read_param_file(file_path))

    vec_sizes = [len(v) for v in param_vectors]
    total_size = int(np.prod(vec_sizes))

    # Build meshgrid and flatten
    # Use indexing='ij' so that the first param varies along the first axis
    # (matching ParamSpan.m behavior where param1 varies fastest)
    grids = np.meshgrid(*param_vectors, indexing="ij")

    spanned: dict[str, np.ndarray] = {}
    for i, name in enumerate(param_names):
        # Flatten in Fortran order: first index varies fastest
        flat = grids[i].ravel(order="F")
        spanned[name] = flat

        # Write span file
        span_path = os.path.join(config_dir, f"{name}_span")
        # Write one value per line (CSV format)
        if np.issubdtype(flat.dtype, np.number):
            np.savetxt(span_path, flat, fmt="%10.6e")
        else:
            with open(span_path, "w") as f:
                f.write("\n".join(str(v) for v in flat))

    spanned["param_space_dims"] = vec_sizes
    spanned["param_names_ordered"] = param_names
    return spanned
