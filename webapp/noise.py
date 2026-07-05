"""Shared noise-data generation and browsing.

Noise data is large and shared by all users, so it lives in one global store
(settings.NOISEDATA_DIR, i.e. Playground/NoiseData) rather than per user. Each
group is a subdirectory holding its channel CSVs plus a "<tag>_config.json"
backup (NoiseGen copies the config there), which is what we show on click.

Generation runs the NoiseGen binary (cwd = Playground so "./NoiseData/..."
resolves) and reports progress by counting the per-channel
"noise data write to: ...#<i>.csv" lines it prints.
"""
import asyncio
import json
import re
import shutil
import subprocess
import threading
import uuid
from collections import deque
from pathlib import Path
from typing import Optional

from fastapi import APIRouter, Body, Depends, HTTPException, Request
from fastapi.responses import StreamingResponse

from . import schema, settings
from .auth import get_current_user

router = APIRouter()

_CHANNEL_RE = re.compile(r"#(\d+)\.csv")
_SAFE_TAG = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,80}$")


def _safe_tag(tag: str) -> str:
    if not isinstance(tag, str) or not _SAFE_TAG.match(tag):
        raise HTTPException(status_code=400, detail=f"Invalid noise tag: {tag!r}")
    return tag


def _group_config(d: Path) -> Optional[dict]:
    for cfg in sorted(d.glob("*_config.json")):
        try:
            return json.loads(cfg.read_text())
        except (json.JSONDecodeError, OSError):
            return None
    return None


# --------------------------------------------------------------------------
# Generation job management
# --------------------------------------------------------------------------
class NoiseJob:
    def __init__(self, job_id: str, tag: str, channels: int):
        self.job_id = job_id
        self.tag = tag
        self.channels = channels
        self.done = set()
        self.status = "running"
        self.returncode: Optional[int] = None
        self.log = deque(maxlen=200)
        self.proc: Optional[subprocess.Popen] = None
        self.lock = threading.Lock()

    def snapshot(self) -> dict:
        with self.lock:
            prog = len(self.done)
            pct = round(100.0 * prog / self.channels, 1) if self.channels else None
            return {"job_id": self.job_id, "tag": self.tag, "status": self.status,
                    "progress": prog, "total": self.channels, "percent": pct,
                    "returncode": self.returncode, "log": list(self.log)[-25:]}


class NoiseManager:
    def __init__(self):
        self._jobs = {}
        self._by_tag = {}
        self._lock = threading.Lock()

    def active_tags(self):
        with self._lock:
            return set(self._by_tag)

    def get(self, job_id: str) -> Optional[NoiseJob]:
        with self._lock:
            return self._jobs.get(job_id)

    def start(self, cfg: dict) -> NoiseJob:
        tag = cfg["tag"]
        with self._lock:
            if tag in self._by_tag:
                raise HTTPException(status_code=409,
                                    detail=f"Already generating '{tag}'")
        if not settings.NOISEGEN_BINARY.is_file():
            raise HTTPException(status_code=500,
                                detail=f"NoiseGen not found at {settings.NOISEGEN_BINARY}")
        export_dir = settings.NOISEDATA_DIR / tag
        export_dir.mkdir(parents=True, exist_ok=True)

        run_cfg = dict(cfg)
        run_cfg["export_dir"] = f"./NoiseData/{tag}/"
        run_cfg["length"] = int(cfg["length"])
        # Temp config outside the export dir; NoiseGen copies it inside as
        # <tag>_config.json (copying onto itself would error).
        tmp = settings.NOISEDATA_DIR / f".{tag}.pending.json"
        tmp.write_text(json.dumps(run_cfg, indent=2))

        job = NoiseJob(uuid.uuid4().hex, tag, int(cfg["channels"]))
        try:
            job.proc = subprocess.Popen(
                [str(settings.NOISEGEN_BINARY), "-c", str(tmp)],
                cwd=str(settings.PLAYGROUND_DIR),
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1,
                start_new_session=True)  # survive a server restart
        except OSError as e:
            tmp.unlink(missing_ok=True)
            raise HTTPException(status_code=500, detail=f"Failed to launch NoiseGen: {e}")

        with self._lock:
            self._jobs[job.job_id] = job
            self._by_tag[tag] = job.job_id
        threading.Thread(target=self._reader, args=(job, tmp), daemon=True).start()
        return job

    def _reader(self, job: NoiseJob, tmp: Path):
        for line in job.proc.stdout:
            line = line.rstrip("\n")
            with job.lock:
                job.log.append(line)
                if "write to" in line:
                    m = _CHANNEL_RE.search(line)
                    if m:
                        job.done.add(int(m.group(1)))
        rc = job.proc.wait()
        with job.lock:
            job.returncode = rc
            job.status = "done" if rc == 0 else "error"
        with self._lock:
            self._by_tag.pop(job.tag, None)
        tmp.unlink(missing_ok=True)


_mgr = NoiseManager()


# --------------------------------------------------------------------------
# Endpoints
# --------------------------------------------------------------------------
@router.get("/noise")
def list_noise(user: str = Depends(get_current_user)):
    base = settings.NOISEDATA_DIR
    active = _mgr.active_tags()
    out = []
    if base.is_dir():
        for d in sorted(base.iterdir(), key=lambda p: p.name.lower()):
            if not d.is_dir():
                continue
            cfg = _group_config(d) or {}
            chan_files = list(d.glob("*#*.csv"))
            size = sum(f.stat().st_size for f in d.iterdir() if f.is_file())
            out.append({
                "group": d.name,
                "time_step": cfg.get("time_step"),
                "channels": cfg.get("channels", len(chan_files)),
                "length": cfg.get("length"),
                "mode": cfg.get("mode"),
                "size": size,
                "has_config": bool(_group_config(d)),
                "generating": d.name in active,
            })
    return {"noise": out}


@router.get("/noise/{group}/config")
def get_group_config(group: str, user: str = Depends(get_current_user)):
    d = settings.NOISEDATA_DIR / _safe_tag(group)
    if not d.is_dir():
        raise HTTPException(status_code=404, detail="Noise group not found")
    cfg = _group_config(d)
    if cfg is None:
        raise HTTPException(status_code=404, detail="No saved config for this group")
    return cfg


@router.post("/noise/generate")
def generate(cfg: dict = Body(...), user: str = Depends(get_current_user)):
    try:
        clean = schema.validate_noise_config(cfg)
    except schema.ValidationError as e:
        raise HTTPException(status_code=422, detail=str(e))
    _safe_tag(clean["tag"])
    job = _mgr.start(clean)
    return {"job_id": job.job_id, "tag": job.tag, "total": job.channels}


@router.get("/noise/jobs/{job_id}/status")
def job_status(job_id: str, user: str = Depends(get_current_user)):
    job = _mgr.get(job_id)
    if not job:
        raise HTTPException(status_code=404, detail="Job not found")
    return job.snapshot()


@router.get("/noise/jobs/{job_id}/events")
async def job_events(job_id: str, request: Request, user: str = Depends(get_current_user)):
    job = _mgr.get(job_id)
    if not job:
        raise HTTPException(status_code=404, detail="Job not found")

    async def gen():
        last = None
        while True:
            if await request.is_disconnected():
                break
            snap = job.snapshot()
            payload = json.dumps(snap)
            if payload != last:
                yield f"data: {payload}\n\n"
                last = payload
            if snap["status"] in ("done", "error"):
                break
            await asyncio.sleep(0.3)

    return StreamingResponse(gen(), media_type="text/event-stream",
                             headers={"Cache-Control": "no-cache",
                                      "X-Accel-Buffering": "no"})


@router.delete("/noise/{group}")
def delete_noise(group: str, user: str = Depends(get_current_user)):
    tag = _safe_tag(group)
    if tag in _mgr.active_tags():
        raise HTTPException(status_code=409, detail="Generation in progress")
    d = settings.NOISEDATA_DIR / tag
    if not d.is_dir():
        raise HTTPException(status_code=404, detail="Noise group not found")
    shutil.rmtree(d)
    return {"ok": True}
