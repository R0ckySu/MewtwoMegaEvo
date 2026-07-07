"""Plot-data endpoints. Loads a run's HDF5 via the mewtwo package (in its own
Python env) as a subprocess and returns plot-ready JSON for meas_marker.
Also saves rendered plot PNGs into the run folder for later browsing."""
import base64
import json
import re
import subprocess
from typing import Any, Dict

from fastapi import APIRouter, Body, Depends, HTTPException
from fastapi.responses import FileResponse
from pydantic import BaseModel

from . import paths, settings
from .auth import get_current_user

router = APIRouter()

# Allow '=' so slice-annotated names like "Z_Z_line_pw_span=5e-06.png" are valid.
_SAFE_PLOT = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._=-]{0,159}$")


def _extract(run_dir, req: Dict[str, Any]) -> Dict[str, Any]:
    if not settings.MEWTWO_PY.is_file():
        raise HTTPException(
            status_code=500,
            detail=(f"Python interpreter for plotting not found at {settings.MEWTWO_PY}. "
                    "Run `uv sync` at the project root."))
    try:
        proc = subprocess.run(
            [str(settings.MEWTWO_PY), str(settings.PLOT_EXTRACTOR),
             str(run_dir), json.dumps(req)],
            capture_output=True, text=True, timeout=300)
    except subprocess.TimeoutExpired:
        raise HTTPException(status_code=504, detail="Loading the result timed out")
    if proc.returncode != 0:
        tail = (proc.stderr or "").strip().splitlines()[-4:]
        raise HTTPException(status_code=422,
                            detail="Failed to load result: " + " | ".join(tail))
    try:
        return json.loads(proc.stdout)
    except json.JSONDecodeError:
        raise HTTPException(status_code=500, detail="Extractor returned invalid data")


def _run_dir(user: str, name: str):
    d = paths.run_dir(user, name)
    if not d.is_dir():
        raise HTTPException(status_code=404, detail="Run not found")
    return d


@router.get("/runs/{name}/plot/meta")
def plot_meta(name: str, user: str = Depends(get_current_user)):
    return _extract(_run_dir(user, name), {"mode": "meta"})


@router.post("/runs/{name}/plot/data")
def plot_data(name: str, req: Dict[str, Any] = Body(...),
              user: str = Depends(get_current_user)):
    mode = req.get("mode")
    if mode not in ("series", "multiseries", "heatmap"):
        raise HTTPException(status_code=400,
                            detail="mode must be 'series', 'multiseries' or 'heatmap'")
    if not req.get("var"):
        raise HTTPException(status_code=400, detail="var is required")
    if mode == "multiseries" and not req.get("series"):
        raise HTTPException(status_code=400, detail="series dim is required")
    return _extract(_run_dir(user, name), req)


@router.post("/runs/{name}/plot/density")
def plot_density(name: str, req: Dict[str, Any] = Body(default={}),
                 user: str = Depends(get_current_user)):
    """Density-matrix stack (n_markers x dim x dim) for one init/param point."""
    return _extract(_run_dir(user, name),
                    {"mode": "densitymatrix", "init": req.get("init"),
                     "param": req.get("param", 0)})


@router.post("/runs/{name}/plot/densityparam")
def plot_density_param(name: str, req: Dict[str, Any] = Body(default={}),
                       user: str = Depends(get_current_user)):
    """Density matrix at one marker across all params (for the param x-axis)."""
    return _extract(_run_dir(user, name),
                    {"mode": "densityparamstack", "init": req.get("init"),
                     "marker": req.get("marker", 0)})


@router.post("/runs/{name}/plot/live")
def plot_live(name: str, req: Dict[str, Any] = Body(default={}),
              user: str = Depends(get_current_user)):
    """Fold-free partial read for real-time plotting during a run."""
    d = paths.run_dir(user, name)
    if not d.is_dir():
        # the run folder may not exist yet at the very start
        return {"obs": [], "init": [], "vars": [], "n_done": 0, "markers": 0}
    return _extract(d, {"mode": "live", "var": req.get("var")})


# ---- saved plots -----------------------------------------------------------
class SavePlot(BaseModel):
    filename: str
    png: str  # data URL: "data:image/png;base64,...."


@router.post("/runs/{name}/plot/save")
def save_plot(name: str, body: SavePlot, user: str = Depends(get_current_user)):
    d = _run_dir(user, name)
    fn = body.filename
    if not _SAFE_PLOT.match(fn):
        raise HTTPException(status_code=400, detail="Invalid plot name")
    if not fn.lower().endswith(".png"):
        fn += ".png"
    data = body.png.split(",", 1)[-1]
    try:
        raw = base64.b64decode(data)
    except Exception:
        raise HTTPException(status_code=400, detail="Invalid image data")
    plots_dir = d / "plots"
    plots_dir.mkdir(exist_ok=True)
    (plots_dir / fn).write_bytes(raw)
    return {"ok": True, "name": fn}


@router.get("/runs/{name}/plots")
def list_plots(name: str, user: str = Depends(get_current_user)):
    plots_dir = _run_dir(user, name) / "plots"
    out = []
    if plots_dir.is_dir():
        for p in sorted(plots_dir.glob("*.png")):
            out.append({"name": p.name, "mtime": p.stat().st_mtime,
                        "size": p.stat().st_size})
    return {"plots": out}


@router.get("/runs/{name}/plots/{fname}")
def get_plot(name: str, fname: str, user: str = Depends(get_current_user)):
    if not _SAFE_PLOT.match(fname):
        raise HTTPException(status_code=400, detail="Invalid name")
    p = _run_dir(user, name) / "plots" / fname
    if not p.is_file():
        raise HTTPException(status_code=404, detail="Plot not found")
    return FileResponse(p, media_type="image/png")
