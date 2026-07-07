"""Extract plot-ready data from a MewtwoMegaEvo result using the mewtwo loader.

Runs under the mewtwo Python environment (3.10+, with numpy/h5py/xarray). The
FastAPI app invokes this as a subprocess and reads a single JSON object from
stdout. Loader progress messages are routed to stderr so stdout stays clean.

Usage:  python plot_extract.py <run_dir> '<json-request>'

Request modes:
  {"mode": "meta"}
  {"mode": "series",      "var": <v>, "x": <dim>, "fixed": {<dim>: <idx>, ...}}
  {"mode": "multiseries", "var": <v>, "x": <dim>, "series": <dim>, "fixed": {...}}
  {"mode": "heatmap",     "var": <v>, "x": <dim>, "y": <dim>, "fixed": {...}}
"""
import json
import sys

import numpy as np


def _clean(a):
    a = np.asarray(a)
    if np.iscomplexobj(a):
        a = a.real
    a = a.astype(float).ravel()
    return [None if np.isnan(x) or np.isinf(x) else float(x) for x in a]


def _coord_values(ds, dim):
    if dim in ds.coords:
        return _clean(np.asarray(ds.coords[dim].values))
    return list(range(int(ds.sizes[dim])))


def _fmt_label(val):
    """Compact legend label for a coordinate value."""
    if val is None:
        return "n/a"
    if isinstance(val, float):
        return f"{val:g}"
    return str(val)


def _split_var(var, obs_list, init_list):
    rest = var[len("meas_marker_"):] if var.startswith("meas_marker_") else var
    for o in obs_list:
        for i in init_list:
            if f"{o}_{i}" == rest:
                return o, i
    return None


def _live(run_dir, req):
    """Partial, fold-free read of completed parameter points for real-time view.

    Reads /meas_marker/#p/<obs>/<init> for the params done so far, returning a
    (param_index x marker) matrix. Robust to a partially-written HDF5 file.
    """
    import os
    from mewtwo.config import load_all_configs
    from mewtwo.h5_io import get_file_list_with_pattern
    import h5py

    sim, _, _ = load_all_configs(run_dir)
    obs_list = sim.get("observables", [])
    init_list = sim.get("init_states", [])
    task = sim.get("task_name", os.path.basename(run_dir))
    out = {"obs": obs_list, "init": init_list,
           "vars": [f"meas_marker_{o}_{i}" for o in obs_list for i in init_list],
           "n_done": 0, "markers": 0}
    var = req.get("var")
    try:
        files = get_file_list_with_pattern(run_dir, task)
        if not files:
            return out
        with h5py.File(os.path.join(run_dir, files[0]), "r") as f:
            grp = f.get("/meas_marker")
            if grp is None:
                return out
            n = sum(1 for k in grp.keys() if k.startswith("#") and k[1:].isdigit())
            out["n_done"] = n
            oi = _split_var(var, obs_list, init_list) if var else (
                (obs_list[0], init_list[0]) if obs_list and init_list else None)
            if oi and n > 0:
                o, i = oi
                rows = []
                for p in range(n):
                    d = f.get(f"/meas_marker/#{p}/{o}/{i}")
                    if d is None:
                        continue
                    arr = np.asarray(d[:])
                    if arr.ndim > 1:
                        arr = arr[0]
                    rows.append(_clean(arr))
                out["z"] = rows
                out["markers"] = len(rows[0]) if rows else 0
    except (OSError, KeyError):
        pass  # file busy / partially written — report what we have (n_done stays)
    return out


def main():
    run_dir = sys.argv[1]
    req = json.loads(sys.argv[2]) if len(sys.argv) > 2 else {"mode": "meta"}
    mode = req.get("mode", "meta")

    if mode == "live":
        print(json.dumps(_live(run_dir, req)))
        return

    # Keep loader chatter off stdout.
    real_stdout = sys.stdout
    sys.stdout = sys.stderr
    import mewtwo
    ds = mewtwo.load_mewtwo_data(run_dir)
    sys.stdout = real_stdout

    if mode == "meta":
        try:
            from mewtwo.config import load_all_configs
            sim, _, _ = load_all_configs(run_dir)
            obs_all = sim.get("observables", [])
            init_all = sim.get("init_states", [])
        except Exception:
            obs_all, init_all = [], []
        out = {"vars": [], "observables": [], "init_states": []}
        obs_seen, init_seen = [], []
        for v in ds.data_vars:
            if not str(v).startswith("meas_marker_"):
                continue
            da = ds[v]
            dims = [{
                "name": str(d), "size": int(da.sizes[d]),
                "is_coord": d in ds.coords,
                "values": (_coord_values(ds, d) if d in ds.coords else None),
            } for d in da.dims]
            oi = _split_var(str(v), obs_all, init_all)
            obs, init = oi if oi else (str(v)[len("meas_marker_"):], "")
            out["vars"].append({"name": str(v), "label": str(v)[len("meas_marker_"):],
                                "obs": obs, "init": init, "dims": dims})
            if obs not in obs_seen:
                obs_seen.append(obs)
            if init not in init_seen:
                init_seen.append(init)
        out["observables"] = obs_seen
        out["init_states"] = init_seen
        print(json.dumps(out))
        return

    v = req["var"]
    da = ds[v]

    if mode == "series":
        xdim = req["x"]
        fixed = req.get("fixed", {})
        sel = {d: int(fixed.get(d, 0)) for d in da.dims if d != xdim}
        sub = da.isel(**sel)
        print(json.dumps({"x": _coord_values(ds, xdim), "y": _clean(sub.values),
                          "x_label": xdim, "y_label": v}))
        return

    if mode == "multiseries":
        # One line per value of `series` dim (a legend); `x` is the shared axis.
        xdim, sdim = req["x"], req["series"]
        fixed = req.get("fixed", {})
        sel = {d: int(fixed.get(d, 0)) for d in da.dims if d not in (xdim, sdim)}
        sub = da.isel(**sel)  # now spans only xdim and sdim
        n_s = int(ds.sizes[sdim])
        svals = _coord_values(ds, sdim)
        # Cap the number of legend traces (subsample evenly) so it stays readable.
        max_traces = int(req.get("max_traces", 40))
        if n_s > max_traces:
            idxs = sorted({int(round(k * (n_s - 1) / (max_traces - 1)))
                           for k in range(max_traces)})
        else:
            idxs = list(range(n_s))
        series = []
        for k in idxs:
            line = sub.isel(**{sdim: k})
            series.append({"name": _fmt_label(svals[k] if k < len(svals) else k),
                           "y": _clean(line.values)})
        print(json.dumps({
            "x": _coord_values(ds, xdim), "series": series,
            "x_label": xdim, "y_label": v, "series_label": sdim,
            "truncated": len(idxs) < n_s, "n_series_total": n_s}))
        return

    if mode == "heatmap":
        xdim, ydim = req["x"], req["y"]
        fixed = req.get("fixed", {})
        sel = {d: int(fixed.get(d, 0)) for d in da.dims if d not in (xdim, ydim)}
        sub = da.isel(**sel).transpose(ydim, xdim)
        z = np.asarray(sub.values)
        if np.iscomplexobj(z):
            z = z.real
        print(json.dumps({
            "x": _coord_values(ds, xdim), "y": _coord_values(ds, ydim),
            "z": [_clean(row) for row in z],
            "x_label": xdim, "y_label": ydim, "z_label": v}))
        return

    raise SystemExit(f"Unknown mode: {mode}")


if __name__ == "__main__":
    main()
