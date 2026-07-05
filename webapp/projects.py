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
            })
    runs.sort(key=lambda r: r["mtime"], reverse=True)
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
