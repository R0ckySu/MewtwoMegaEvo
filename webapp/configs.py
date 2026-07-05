"""Read/write the user's three JSON config files, validated against schema.py."""
import json
from typing import Any, Dict, Set

from fastapi import APIRouter, Body, Depends, HTTPException

from . import paths, schema
from .auth import get_current_user

router = APIRouter()

_STARTER = {
    "sim_config.json": {
        "task_name": "NewTask", "log_level": 4, "job_slicing_strategy": "linspace",
        "record_propagator": False, "record_all_meas": False, "record_density_mat": True,
        "reset_rotating_frame": False, "enable_param_parallel_mode": True,
        "system_dim": 2, "observables": [], "init_states": [], "repeat": 1,
        "step_size": 1e-6, "sequence": "", "sweep_param_info": [],
    },
    "gate_config.json": {"gate_defs": []},
    "hamiltonian_config.json": {"hamiltonian_prototype_defs": []},
}


def ensure_starter_configs(cfg_dir) -> None:
    """Create empty valid configs if the user has none yet."""
    cfg_dir.mkdir(parents=True, exist_ok=True)
    for fname, content in _STARTER.items():
        p = cfg_dir / fname
        if not p.exists():
            p.write_text(json.dumps(content, indent=2))


def _config_path(user: str, kind: str):
    if kind not in schema.CONFIG_FILENAMES:
        raise HTTPException(status_code=404, detail=f"Unknown config kind: {kind}")
    cfg_dir = paths.config_dir(user)
    ensure_starter_configs(cfg_dir)
    return cfg_dir / schema.CONFIG_FILENAMES[kind]


@router.get("/config/{kind}")
def read_config(kind: str, user: str = Depends(get_current_user)):
    path = _config_path(user, kind)
    try:
        return json.loads(path.read_text())
    except json.JSONDecodeError as e:
        raise HTTPException(status_code=422, detail=f"{path.name} is not valid JSON: {e}")


@router.put("/config/{kind}")
def write_config(kind: str, data: Dict[str, Any] = Body(...),
                 user: str = Depends(get_current_user)):
    path = _config_path(user, kind)
    try:
        cleaned = schema.VALIDATORS[kind](data)
    except schema.ValidationError as e:
        raise HTTPException(status_code=422, detail=str(e))
    path.write_text(json.dumps(cleaned, indent=2))
    return {"ok": True, "saved": path.name}


def collect_referenced_files(cfg_dir):
    """Filenames referenced by the three configs.

    Returns (referenced, strict): `referenced` is every filename mentioned by a
    file-ref field (used to tag existing files); `strict` is only those from
    fields that are always plain filenames (used to flag missing files, so
    built-in Pauli symbols like "X"/"IZ" are not reported as missing).
    """
    referenced: Set[str] = set()
    strict: Set[str] = set()

    def scan(obj: Any):
        if isinstance(obj, dict):
            for k, v in obj.items():
                if k in schema.FILE_REF_FIELDS and isinstance(v, str) and v:
                    name = v.lstrip("./")
                    referenced.add(name)
                    if k in schema.STRICT_FILE_FIELDS:
                        strict.add(name)
                else:
                    scan(v)
        elif isinstance(obj, list):
            for x in obj:
                scan(x)

    for fname in schema.CONFIG_FILENAMES.values():
        p = cfg_dir / fname
        if p.is_file():
            try:
                scan(json.loads(p.read_text()))
            except json.JSONDecodeError:
                pass
    return referenced, strict
