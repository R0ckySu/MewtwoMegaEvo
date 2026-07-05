"""Download or delete a run folder (directly under the user's folder)."""
import shutil
import tempfile
from pathlib import Path

from fastapi import APIRouter, BackgroundTasks, Depends, HTTPException
from fastapi.responses import FileResponse

from . import paths
from .auth import get_current_user

router = APIRouter()


@router.get("/runs/{name}/download")
def download_run(name: str, background: BackgroundTasks,
                 user: str = Depends(get_current_user)):
    folder = paths.run_dir(user, name)
    if not folder.is_dir():
        raise HTTPException(status_code=404, detail="Run not found")
    tmp_base = Path(tempfile.mkdtemp()) / name
    archive = shutil.make_archive(str(tmp_base), "zip", root_dir=str(folder))
    background.add_task(shutil.rmtree, str(tmp_base.parent), ignore_errors=True)
    return FileResponse(archive, filename=f"{name}.zip", media_type="application/zip")


@router.delete("/runs/{name}")
def delete_run(name: str, user: str = Depends(get_current_user)):
    folder = paths.run_dir(user, name)
    if not folder.is_dir():
        raise HTTPException(status_code=404, detail="Run not found")
    shutil.rmtree(folder)
    return {"ok": True}
