"""Config sourcing: list demo templates, load a demo into the user's config,
reset to blank, and migrate the current config to the binary's required fields.
"""
import shutil

from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel

from . import configs, migrate, paths, settings
from .auth import get_current_user

router = APIRouter()


def _demo_dirs():
    if not settings.DEMO_CONFIGS_DIR.is_dir():
        return []
    return [d.name for d in sorted(settings.DEMO_CONFIGS_DIR.iterdir())
            if d.is_dir() and (d / "sim_config.json").is_file()]


@router.get("/demos")
def list_demos(user: str = Depends(get_current_user)):
    return {"demos": _demo_dirs()}


class LoadDemo(BaseModel):
    demo: str


@router.post("/load_demo")
def load_demo(body: LoadDemo, user: str = Depends(get_current_user)):
    """Replace the user's editable config with a demo template."""
    demo = paths.safe_name(body.demo, "demo")
    src = settings.DEMO_CONFIGS_DIR / demo
    if not (src / "sim_config.json").is_file():
        raise HTTPException(status_code=404, detail=f"Demo not found: {demo}")
    cfg = paths.config_dir(user)
    if cfg.exists():
        shutil.rmtree(cfg)
    cfg.mkdir(parents=True, exist_ok=True)
    for item in src.iterdir():
        if item.is_file():
            shutil.copy2(item, cfg / item.name)
        elif item.is_dir():
            shutil.copytree(item, cfg / item.name, dirs_exist_ok=True)
    return {"migration": migrate.migrate_config_dir(cfg)}


@router.post("/config/reset")
def reset_config(user: str = Depends(get_current_user)):
    """Clear the config back to blank starter files."""
    cfg = paths.config_dir(user)
    if cfg.exists():
        shutil.rmtree(cfg)
    configs.ensure_starter_configs(cfg)
    return {"ok": True}


@router.post("/migrate")
def migrate_config(user: str = Depends(get_current_user)):
    """Fill required fields the current binary expects; report remaining blockers."""
    cfg = paths.ensure_config_dir(user)
    return migrate.migrate_config_dir(cfg)
