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


def _density_param_map(run_dir):
    """Flat list of (file, '#p' key) for every recorded density-matrix param,
    ordered by global param index, plus the init states available."""
    import os
    from mewtwo.config import load_all_configs
    from mewtwo.h5_io import get_file_list_with_pattern
    import h5py

    sim, _, _ = load_all_configs(run_dir)
    task = sim.get("task_name", os.path.basename(run_dir))
    inits = sim.get("init_states", [])
    flat = []
    for fn in get_file_list_with_pattern(run_dir, task):
        try:
            with h5py.File(os.path.join(run_dir, fn), "r") as f:
                g = f.get("/rho_marker")
                if g is None:
                    continue
                keys = sorted((k for k in g.keys()
                               if k.startswith("#") and k[1:].isdigit()),
                              key=lambda k: int(k[1:]))
                flat += [(fn, k) for k in keys]
        except OSError:
            pass
    return flat, inits


def _density(run_dir, req):
    """Full density-matrix stack for one init state + param point:
    (n_markers x dim x dim) split into real/imag, for client-side slicing."""
    import os
    from mewtwo.h5_io import read_complex_dataset
    import h5py

    flat, inits = _density_param_map(run_dir)
    init = req.get("init") or (inits[0] if inits else "")
    out = {"inits": inits, "init": init, "n_params": len(flat),
           "param": 0, "dim": 0, "n_markers": 0, "markers": [],
           "re": [], "im": []}
    if not flat or not init:
        return out
    p = max(0, min(int(req.get("param", 0) or 0), len(flat) - 1))
    out["param"] = p
    fn, key = flat[p]
    try:
        with h5py.File(os.path.join(run_dir, fn), "r") as f:
            path = f"/rho_marker/{key}/{init}"
            if path not in f:
                return out
            c = np.asarray(read_complex_dataset(f, path))
            if c.ndim == 2:            # single marker -> (1, dim, dim)
                c = c[None, :, :]
            if c.ndim != 3:
                return out
            m, d, _ = c.shape
            out["dim"] = int(d)
            out["n_markers"] = int(m)
            out["markers"] = list(range(int(m)))
            re = np.real(c)
            im = np.imag(c)
            re[~np.isfinite(re)] = 0.0
            im[~np.isfinite(im)] = 0.0
            out["re"] = re.astype(float).tolist()
            out["im"] = im.astype(float).tolist()
    except OSError:
        pass
    return out


def _density_param_stack(run_dir, req):
    """Density matrix at ONE marker across ALL params: (n_params x dim x dim),
    for tracing an element against the swept parameter."""
    import os
    from mewtwo.h5_io import read_complex_dataset
    import h5py

    flat, inits = _density_param_map(run_dir)
    init = req.get("init") or (inits[0] if inits else "")
    marker = int(req.get("marker", 0) or 0)
    out = {"init": init, "marker": marker, "n_params": len(flat),
           "dim": 0, "re": [], "im": []}
    if not flat or not init:
        return out
    re_all = [None] * len(flat)
    im_all = [None] * len(flat)
    dim = 0
    by_file = {}
    for i, (fn, key) in enumerate(flat):
        by_file.setdefault(fn, []).append((i, key))
    for fn, items in by_file.items():
        try:
            with h5py.File(os.path.join(run_dir, fn), "r") as f:
                for i, key in items:
                    path = f"/rho_marker/{key}/{init}"
                    if path not in f:
                        continue
                    c = np.asarray(read_complex_dataset(f, path))
                    if c.ndim == 2:
                        c = c[None, :, :]
                    m = min(marker, c.shape[0] - 1)
                    mat = c[m]
                    dim = mat.shape[0]
                    re = np.real(mat).astype(float)
                    im = np.imag(mat).astype(float)
                    re[~np.isfinite(re)] = 0.0
                    im[~np.isfinite(im)] = 0.0
                    re_all[i] = re.tolist()
                    im_all[i] = im.tolist()
        except OSError:
            pass
    out["dim"] = int(dim)
    out["re"] = re_all
    out["im"] = im_all
    return out


def main():
    run_dir = sys.argv[1]
    req = json.loads(sys.argv[2]) if len(sys.argv) > 2 else {"mode": "meta"}
    mode = req.get("mode", "meta")

    if mode == "live":
        print(json.dumps(_live(run_dir, req)))
        return

    if mode == "densitymatrix":
        print(json.dumps(_density(run_dir, req)))
        return

    if mode == "densityparamstack":
        print(json.dumps(_density_param_stack(run_dir, req)))
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
        # Density-matrix availability (raw HDF5 check, independent of the loader).
        try:
            flat, dinits = _density_param_map(run_dir)
            out["has_density"] = bool(flat)
            out["density_inits"] = dinits if flat else []
            out["density_n_params"] = len(flat)
        except Exception:
            out["has_density"] = False
            out["density_inits"] = []
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
