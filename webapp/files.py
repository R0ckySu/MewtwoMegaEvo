"""Browse/edit the matrix, vector and parameter files in the user's config dir.

These are the plain-text files referenced by the JSON configs (comma-separated
matrix rows, newline-separated vectors, sequence lists, etc.).
"""
from fastapi import APIRouter, Body, Depends, HTTPException
from pydantic import BaseModel

from . import paths, schema
from .auth import get_current_user
from .configs import collect_referenced_files

router = APIRouter()

MAX_EDIT_BYTES = 2_000_000  # files larger than this are read-only in the editor
_HIDDEN = {".DS_Store"}
_CONFIG_JSONS = set(schema.CONFIG_FILENAMES.values())


@router.get("/files")
def list_files(user: str = Depends(get_current_user)):
    cfg = paths.ensure_config_dir(user)
    referenced, strict = collect_referenced_files(cfg)
    out = []
    for p in sorted(cfg.iterdir()):
        if not p.is_file() or p.name in _HIDDEN or p.name in _CONFIG_JSONS:
            continue
        size = p.stat().st_size
        out.append({
            "name": p.name,
            "size": size,
            "referenced": p.name in referenced,
            "editable": size <= MAX_EDIT_BYTES,
        })
    existing = {f["name"] for f in out}
    missing = [{"name": n, "size": 0, "referenced": True, "editable": False,
                "missing": True} for n in sorted(strict) if n not in existing]
    return {"files": out + missing}


@router.get("/files/{name}")
def read_file(name: str, user: str = Depends(get_current_user)):
    path = paths.resolve_in_config(user, name)
    if not path.is_file():
        raise HTTPException(status_code=404, detail="File not found")
    size = path.stat().st_size
    if size > MAX_EDIT_BYTES:
        return {"name": name, "size": size, "truncated": True,
                "content": path.read_bytes()[:MAX_EDIT_BYTES].decode("utf-8", "replace")}
    return {"name": name, "size": size, "truncated": False,
            "content": path.read_text(errors="replace")}


@router.put("/files/{name}")
def write_file(name: str, content: str = Body(..., embed=True),
               user: str = Depends(get_current_user)):
    path = paths.resolve_in_config(user, name)
    paths.ensure_config_dir(user)
    path.write_text(content)
    return {"ok": True, "name": name, "size": path.stat().st_size}


class NewFile(BaseModel):
    name: str
    content: str = ""


@router.post("/files")
def create_file(body: NewFile, user: str = Depends(get_current_user)):
    path = paths.resolve_in_config(user, body.name)
    paths.ensure_config_dir(user)
    if path.exists():
        raise HTTPException(status_code=409, detail="File already exists")
    path.write_text(body.content)
    return {"ok": True, "name": body.name}


@router.delete("/files/{name}")
def delete_file(name: str, user: str = Depends(get_current_user)):
    path = paths.resolve_in_config(user, name)
    if not path.is_file():
        raise HTTPException(status_code=404, detail="File not found")
    path.unlink()
    return {"ok": True}
