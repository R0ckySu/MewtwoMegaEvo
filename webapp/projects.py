"""Per-user project portal: browse previous runs and load a past run's config
back into the editable config to iterate on it.

Runs live directly under the user's folder as "<task><timestamp>" directories,
each holding a config_files/ backup (which is what "load config" restores).
"""
import json
import re
import shutil

from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel

from . import migrate, paths
from .auth import get_current_user

router = APIRouter()

# Timestamp appended by the runner: strftime("%Y%m%d%H%M%S") -> 14 digits.
_TS_RE = re.compile(r"\d{14}$")


def strip_timestamp(folder_name: str) -> str:
    return _TS_RE.sub("", folder_name) or folder_name


def _is_run_dir(d) -> bool:
    return d.is_dir() and d.name != paths.CONFIG_DIRNAME and not d.name.startswith(".")


def _result_files(d):
    """Output data files (the binary writes '<task><ts>_Job#<N>', no extension)."""
    return [f.name for f in d.iterdir()
            if f.is_file() and ("_Job#" in f.name or f.suffix == ".h5")]


def _saved_plots(d):
    """Names of PNGs saved under the run's plots/ folder (for the gallery)."""
    pdir = d / "plots"
    if not pdir.is_dir():
        return []
    return sorted(p.name for p in pdir.glob("*.png"))


# Scalar sim-config keys worth reporting a change on.
_DIFF_KEYS = [
    ("log_level", "log_level"), ("step_size", "step_size"), ("repeat", "repeat"),
    ("system_dim", "system_dim"), ("sequence", "sequence"),
    ("job_slicing_strategy", "slicing"),
    ("record_propagator", "record_propagator"), ("record_all_meas", "record_all_meas"),
    ("record_density_mat", "record_density_mat"),
    ("reset_rotating_frame", "reset_rotating_frame"),
    ("enable_param_parallel_mode", "param_parallel"),
]


def _num_stats(path):
    """(count, min, max) of a numeric one-per-line file, or None."""
    try:
        vals = [float(x) for x in path.read_text().split() if x.strip()]
    except (OSError, ValueError):
        return None
    return (len(vals), min(vals), max(vals)) if vals else None


def _config_summary(cfg_dir):
    """Flat, comparable snapshot of a run's config (for neighbour diffs)."""
    s = {}
    try:
        sim = json.loads((cfg_dir / "sim_config.json").read_text())
    except (OSError, json.JSONDecodeError):
        return s
    for key, label in _DIFF_KEYS:
        if key in sim:
            s[label] = sim[key]
    s["observables"] = ",".join(sim.get("observables", []) or [])
    s["init_states"] = ",".join(sim.get("init_states", []) or [])
    # swept parameter files: length + range (captures a changed sweep range)
    for row in sim.get("sweep_param_info", []) or []:
        fname = row.get("val_file") or row.get("string_file")
        tag = f"sweep[{row.get('tag', '')}.{row.get('property', '')}]"
        if fname:
            st = _num_stats(cfg_dir / fname)
            s[tag] = (f"n={st[0]}, {st[1]:g}..{st[2]:g}" if st else fname)
    for cfg, label in (("gate_config.json", "gates"),
                       ("hamiltonian_config.json", "hamiltonians")):
        try:
            data = json.loads((cfg_dir / cfg).read_text())
            key = "gate_defs" if "gate" in cfg else "hamiltonian_prototype_defs"
            s[f"#{label}"] = len(data.get(key, []) or [])
        except (OSError, json.JSONDecodeError):
            pass
    return s


def _fmt_val(v):
    if isinstance(v, float):
        return f"{v:g}"
    if isinstance(v, bool):
        return "on" if v else "off"
    return str(v)


def _diff_summaries(cur, prev):
    """Short human list of what changed from prev -> cur."""
    out = []
    for k in cur:
        if k in prev and prev[k] != cur[k]:
            out.append(f"{k}: {_fmt_val(prev[k])} → {_fmt_val(cur[k])}")
        elif k not in prev:
            out.append(f"{k}: (new) {_fmt_val(cur[k])}")
    for k in prev:
        if k not in cur:
            out.append(f"{k}: removed")
    return out[:8]


@router.get("/runs")
def list_runs(user: str = Depends(get_current_user)):
    root = paths.user_root(user)
    runs = []
    if root.is_dir():
        for d in root.iterdir():
            if not _is_run_dir(d):
                continue
            size = sum(f.stat().st_size for f in d.rglob("*") if f.is_file())
            runs.append({
                "name": d.name, "task_name": strip_timestamp(d.name),
                "mtime": d.stat().st_mtime, "size": size,
                "h5_files": _result_files(d),
                "has_config": (d / "config_files").is_dir(),
                "plots": _saved_plots(d),
                "dir": d,  # transient, dropped before returning
            })
    runs.sort(key=lambda r: r["mtime"], reverse=True)

    # Change notes: compare each run to the most recent OLDER run with the same
    # task name (runs are newest-first, so that's the next same-task entry).
    for i, r in enumerate(runs):
        r["changes"] = None
        r["compared_to"] = None
        prev = next((runs[j] for j in range(i + 1, len(runs))
                     if runs[j]["task_name"] == r["task_name"]), None)
        if prev is not None and r["has_config"] and prev["has_config"]:
            diff = _diff_summaries(
                _config_summary(r["dir"] / "config_files"),
                _config_summary(prev["dir"] / "config_files"))
            if diff:
                r["changes"] = diff
                r["compared_to"] = prev["name"]
    for r in runs:
        r.pop("dir", None)
    return {"runs": runs}


class CopyRun(BaseModel):
    run_name: str


@router.post("/runs/copy")
def load_run_config(body: CopyRun, user: str = Depends(get_current_user)):
    """Replace the editable config_files with a past run's saved config."""
    src = paths.run_dir(user, body.run_name) / "config_files"
    if not src.is_dir():
        raise HTTPException(status_code=404, detail="Run has no saved config_files")

    cfg = paths.config_dir(user)
    if cfg.exists():
        shutil.rmtree(cfg)
    cfg.mkdir(parents=True, exist_ok=True)
    for item in src.iterdir():
        if item.is_file():
            shutil.copy2(item, cfg / item.name)
        elif item.is_dir():
            shutil.copytree(item, cfg / item.name, dirs_exist_ok=True)

    report = migrate.migrate_config_dir(cfg)
    return {"task_name": strip_timestamp(body.run_name), "migration": report}
