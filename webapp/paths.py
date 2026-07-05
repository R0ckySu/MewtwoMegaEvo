"""Safe per-user path resolution.

Layout mirrors Playground, one sandbox per user:

    userdata/<user>/config_files/          <- the single editable config
    userdata/<user>/<task><timestamp>/     <- each run's output (directly here)

Every user-supplied path component is validated to a single safe name so a
request can never escape the user's data directory.
"""
import re
from pathlib import Path

from fastapi import HTTPException

from . import settings

_SAFE_NAME = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,79}$")

CONFIG_DIRNAME = "config_files"


def safe_name(name: str, what: str = "name") -> str:
    if not isinstance(name, str) or not _SAFE_NAME.match(name) or name in (".", ".."):
        raise HTTPException(status_code=400, detail=f"Invalid {what}: {name!r}")
    return name


def user_root(user: str) -> Path:
    return settings.USERDATA_DIR / safe_name(user, "user")


def config_dir(user: str) -> Path:
    return user_root(user) / CONFIG_DIRNAME


def run_dir(user: str, name: str) -> Path:
    return user_root(user) / safe_name(name, "run")


def resolve_in_config(user: str, filename: str) -> Path:
    """Resolve a filename inside the user's config_files dir, safely."""
    return config_dir(user) / safe_name(filename, "filename")


def ensure_config_dir(user: str) -> Path:
    d = config_dir(user)
    d.mkdir(parents=True, exist_ok=True)
    return d
