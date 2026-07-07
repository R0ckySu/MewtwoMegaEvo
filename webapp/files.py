"""Browse/edit the matrix, vector and parameter files in the user's config dir.

These are the plain-text files referenced by the JSON configs (comma-separated
matrix rows, newline-separated vectors, sequence lists, etc.).
"""
import os
import re

import numpy as np
from fastapi import APIRouter, Body, Depends, File, HTTPException, UploadFile
from pydantic import BaseModel

from mewtwo.params import param_span
from . import paths, schema
from .auth import get_current_user
from .configs import collect_referenced_files

router = APIRouter()

MAX_EDIT_BYTES = 2_000_000     # files larger than this are read-only in the editor
MAX_UPLOAD_BYTES = 64_000_000  # per uploaded file
_HIDDEN = {".DS_Store"}
_CONFIG_JSONS = set(schema.CONFIG_FILENAMES.values())


def _safe_upload_name(filename: str) -> str:
    base = os.path.basename(filename or "")
    base = re.sub(r"[^A-Za-z0-9._-]", "_", base)
    base = re.sub(r"^[^A-Za-z0-9]+", "", base)[:80]
    return base or "upload"


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


@router.post("/files/upload")
async def upload_files(files: list[UploadFile] = File(...),
                       user: str = Depends(get_current_user)):
    """Upload one or more local files into the user's config dir (overwrites)."""
    cfg = paths.ensure_config_dir(user)
    saved = []
    for f in files:
        name = _safe_upload_name(f.filename)
        data = await f.read()
        if len(data) > MAX_UPLOAD_BYTES:
            raise HTTPException(status_code=413,
                                detail=f"{name} exceeds the 64 MB upload limit")
        (cfg / name).write_bytes(data)
        saved.append(name)
    return {"ok": True, "saved": saved}


MAX_VECTOR_POINTS = 5_000_000


class VectorSpec(BaseModel):
    name: str
    method: str = "linspace"        # linspace | logspace
    start: float
    stop: float
    num: int


@router.post("/files/vector")
def create_vector(body: VectorSpec, user: str = Depends(get_current_user)):
    """Generate a numerical vector file (one value per line) via linspace or a
    geometric logspace between start and stop."""
    if body.num < 1 or body.num > MAX_VECTOR_POINTS:
        raise HTTPException(status_code=400,
                            detail=f"num must be between 1 and {MAX_VECTOR_POINTS}")
    if body.method == "linspace":
        vals = np.linspace(body.start, body.stop, body.num)
    elif body.method == "logspace":
        if body.start <= 0 or body.stop <= 0:
            raise HTTPException(status_code=400,
                                detail="logspace needs start and stop > 0")
        vals = np.logspace(np.log10(body.start), np.log10(body.stop), body.num)
    else:
        raise HTTPException(status_code=400,
                            detail="method must be 'linspace' or 'logspace'")
    path = paths.resolve_in_config(user, body.name)
    paths.ensure_config_dir(user)
    path.write_text("\n".join(f"{v:.12g}" for v in vals) + "\n")
    return {"ok": True, "name": body.name, "count": int(len(vals)),
            "first": float(vals[0]), "last": float(vals[-1])}


class SpanSpec(BaseModel):
    names: list[str]


@router.post("/files/span")
def span_vectors(body: SpanSpec, user: str = Depends(get_current_user)):
    """Tensor several vector files (in order, first varies fastest) into a
    flattened N-D meshgrid, writing a <name>_span file for each input."""
    if len(body.names) < 2:
        raise HTTPException(status_code=400,
                            detail="Select at least two vector files to span")
    cfg = paths.ensure_config_dir(user)
    dims, total = [], 1
    for n in body.names:
        p = paths.resolve_in_config(user, n)      # validates the filename
        if n.endswith("_span"):
            raise HTTPException(status_code=400,
                                detail=f"'{n}' is already a span file — pick base vectors")
        if not p.is_file():
            raise HTTPException(status_code=404, detail=f"File not found: {n}")
        k = sum(1 for ln in p.read_text().split("\n") if ln.strip())
        dims.append(k)
        total *= max(k, 1)
    if total > MAX_VECTOR_POINTS:
        raise HTTPException(status_code=400,
                            detail=f"Meshgrid too large ({total} points)")
    try:
        result = param_span(str(cfg), list(body.names))
    except Exception as e:
        raise HTTPException(status_code=422, detail=f"Span failed: {e}")
    return {"ok": True, "created": [f"{n}_span" for n in body.names],
            "dims": result.get("param_space_dims", dims), "total": total}


@router.delete("/files/{name}")
def delete_file(name: str, user: str = Depends(get_current_user)):
    path = paths.resolve_in_config(user, name)
    if not path.is_file():
        raise HTTPException(status_code=404, detail="File not found")
    path.unlink()
    return {"ok": True}
