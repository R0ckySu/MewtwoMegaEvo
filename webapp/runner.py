"""Global single-job queue for simulations.

Only one simulation runs on the server at a time; further submissions queue
(FIFO) and users see their position and an ETA. Each job snapshots the user's
config at submit time (so later edits don't affect a queued job), runs it in its
own session with stdout to a log file (survives a server restart), and writes
results to  userdata/<user>/<task><timestamp>/ .

ETA model: a global EMA of seconds-per-work-unit is learned from completed jobs.
Work units = num_params * repeat. The running job's remaining time uses its own
observed rate; queued jobs use the EMA times their estimated units, summed over
everything ahead of them.
"""
import asyncio
import json
import re
import shutil
import subprocess
import threading
import time
import uuid
from collections import deque
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional

from fastapi import APIRouter, Depends, HTTPException, Request
from fastapi.responses import StreamingResponse

from . import configs, paths, settings
from .auth import get_current_user

router = APIRouter()

_PROGRESS_RE = re.compile(r"Solver job done! \[(\d+)/(\d+)\]")


def _estimate_units(cfg_dir: Path) -> int:
    """num_params * repeat, from the (snapshotted) sim config."""
    try:
        sim = json.loads((cfg_dir / "sim_config.json").read_text())
    except (OSError, json.JSONDecodeError):
        return 1
    repeat = max(int(sim.get("repeat", 1) or 1), 1)
    n = 1
    for s in sim.get("sweep_param_info", []) or []:
        fname = s.get("val_file") or s.get("string_file")
        if fname and (cfg_dir / fname).is_file():
            try:
                n = sum(1 for ln in (cfg_dir / fname).read_text().splitlines() if ln.strip())
            except OSError:
                n = 1
            break  # spanned vectors all share the same length
    return max(n, 1) * repeat


class Job:
    def __init__(self, job_id, user, task_name, timestamp, output_root, cfg_snapshot, units):
        self.job_id = job_id
        self.user = user
        self.task_name = task_name
        self.timestamp = timestamp
        self.output_root = output_root
        self.cfg_snapshot = cfg_snapshot  # Path to .queued/<id>/config_files
        self.units = units
        self.status = "queued"  # queued|running|done|error|stopped
        self.progress = 0
        self.total: Optional[int] = None
        self.submitted_at = time.time()
        self.started_at: Optional[float] = None
        self.finished_at: Optional[float] = None
        self.returncode: Optional[int] = None
        self.log = deque(maxlen=300)
        self.proc: Optional[subprocess.Popen] = None
        self.lock = threading.Lock()

    def running_eta(self) -> Optional[float]:
        if self.status != "running" or not self.total or self.progress <= 0 or not self.started_at:
            return None
        rate = (time.time() - self.started_at) / self.progress
        return max(0.0, (self.total - self.progress) * rate)

    def base(self) -> dict:
        with self.lock:
            pct = round(100.0 * self.progress / self.total, 1) if self.total else None
            out_dir = None
            if self.status == "done":
                cand = self.output_root / f"{self.task_name}{self.timestamp}"
                if cand.is_dir():
                    out_dir = cand.name
            return {
                "job_id": self.job_id, "user": self.user, "task_name": self.task_name,
                "status": self.status, "progress": self.progress, "total": self.total,
                "percent": pct, "returncode": self.returncode, "timestamp": self.timestamp,
                "run_folder": f"{self.task_name}{self.timestamp}", "output_dir": out_dir,
                "submitted_at": self.submitted_at, "started_at": self.started_at,
                "log": list(self.log)[-40:],
            }


class QueueManager:
    def __init__(self):
        self._jobs: Dict[str, Job] = {}
        self._queue: List[str] = []
        self._current: Optional[str] = None
        self._secs_per_unit: Optional[float] = None
        self._lock = threading.Lock()

    # ---- submission / lifecycle ----
    def submit(self, user: str) -> Job:
        cfg_dir = paths.config_dir(user)
        configs.ensure_starter_configs(cfg_dir)
        if not settings.BINARY_PATH.is_file():
            raise HTTPException(status_code=500,
                                detail=f"Simulator binary not found at {settings.BINARY_PATH}")
        try:
            task_name = json.loads((cfg_dir / "sim_config.json").read_text()).get(
                "task_name", "task")
        except (OSError, json.JSONDecodeError):
            task_name = "task"

        job_id = uuid.uuid4().hex
        timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
        # snapshot the config so later edits don't change a queued job
        snap = paths.user_root(user) / ".queued" / job_id / "config_files"
        snap.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(cfg_dir, snap)
        job = Job(job_id, user, task_name, timestamp, paths.user_root(user), snap,
                  _estimate_units(snap))
        with self._lock:
            self._jobs[job_id] = job
            self._queue.append(job_id)
        self._start_next()
        return job

    def _start_next(self):
        with self._lock:
            if self._current is not None or not self._queue:
                return
            job = self._jobs[self._queue.pop(0)]
            self._current = job.job_id
        self._launch(job)

    def _launch(self, job: Job):
        cmd = [str(settings.BINARY_PATH), "-c", str(job.cfg_snapshot),
               "-o", str(job.output_root), "-t", job.timestamp]
        log_path = job.output_root / f".run_{job.timestamp}.log"
        try:
            logf = open(log_path, "w")
            job.proc = subprocess.Popen(
                cmd, cwd=str(settings.PLAYGROUND_DIR),
                stdout=logf, stderr=subprocess.STDOUT, start_new_session=True)
            logf.close()
        except OSError as e:
            with job.lock:
                job.status = "error"
                job.log.append(f"Failed to launch: {e}")
            with self._lock:
                self._current = None
            self._start_next()
            return
        with job.lock:
            job.status = "running"
            job.started_at = time.time()
        threading.Thread(target=self._reader, args=(job, log_path), daemon=True).start()

    def _handle_line(self, job: Job, line: str):
        m = _PROGRESS_RE.search(line)
        with job.lock:
            job.log.append(line)
            if m:
                job.progress = int(m.group(1))
                job.total = int(m.group(2))

    def _reader(self, job: Job, log_path: Path):
        while not log_path.exists() and job.proc.poll() is None:
            time.sleep(0.05)
        try:
            f = open(log_path, "r")
        except OSError:
            f = None
        if f:
            while job.proc.poll() is None:
                line = f.readline()
                if line:
                    self._handle_line(job, line.rstrip("\n"))
                else:
                    time.sleep(0.15)
            for line in f.readlines():
                self._handle_line(job, line.rstrip("\n"))
            f.close()
        rc = job.proc.wait()
        with job.lock:
            job.returncode = rc
            job.finished_at = time.time()
            if job.status != "stopped":
                job.status = "done" if rc == 0 else "error"
        # learn seconds-per-unit from a clean completion
        if rc == 0 and job.total and job.started_at:
            sample = (job.finished_at - job.started_at) / max(job.total, 1)
            with self._lock:
                self._secs_per_unit = (sample if self._secs_per_unit is None
                                       else 0.3 * sample + 0.7 * self._secs_per_unit)
        shutil.rmtree(job.cfg_snapshot.parent, ignore_errors=True)
        with self._lock:
            self._current = None
        self._start_next()

    def cancel(self, job: Job):
        with self._lock:
            if job.job_id in self._queue:
                self._queue.remove(job.job_id)
                with job.lock:
                    job.status = "stopped"
                return
        with job.lock:
            if job.proc and job.proc.poll() is None:
                job.status = "stopped"
                job.proc.terminate()

    # ---- views ----
    def get(self, job_id: str) -> Optional[Job]:
        with self._lock:
            return self._jobs.get(job_id)

    def _queue_snapshot(self):
        """(running_view_or_None, [queued_views], eta_by_job_id)."""
        with self._lock:
            spu = self._secs_per_unit
            current = self._jobs.get(self._current) if self._current else None
            queued = [self._jobs[j] for j in self._queue if j in self._jobs]

        eta_by_id = {}
        running_view = None
        cum = 0.0
        have_eta = spu is not None
        if current:
            r = current.running_eta()
            if r is None and have_eta:
                r = current.units * spu
            cum = r or 0.0
            running_view = {**current.base(), "eta_seconds": r}
            eta_by_id[current.job_id] = r
        qviews = []
        for pos, job in enumerate(queued):
            eta_start = cum if have_eta else None
            eta_by_id[job.job_id] = eta_start
            qviews.append({**job.base(), "position": pos + 1, "eta_seconds": eta_start})
            if have_eta:
                cum += job.units * spu
        return running_view, qviews, eta_by_id, spu

    def queue_view(self) -> dict:
        running, qviews, _, spu = self._queue_snapshot()
        return {"running": running, "queue": qviews, "secs_per_unit": spu,
                "queued_count": len(qviews)}

    def job_view(self, job: Job) -> dict:
        running, qviews, eta_by_id, _ = self._queue_snapshot()
        base = job.base()
        base["eta_seconds"] = eta_by_id.get(job.job_id)
        if job.status == "queued":
            base["position"] = next((q["position"] for q in qviews
                                     if q["job_id"] == job.job_id), None)
            base["queued_count"] = len(qviews)
        return base


manager = QueueManager()


@router.post("/run")
def start_run(user: str = Depends(get_current_user)):
    job = manager.submit(user)
    return {"run_id": job.job_id, "timestamp": job.timestamp, "status": job.status}


def _owned(job_id: str, user: str) -> Job:
    job = manager.get(job_id)
    if not job or job.user != user:
        raise HTTPException(status_code=404, detail="Run not found")
    return job


@router.post("/run/{run_id}/stop")
def stop_run(run_id: str, user: str = Depends(get_current_user)):
    manager.cancel(_owned(run_id, user))
    return {"ok": True}


@router.get("/run/{run_id}/status")
def run_status(run_id: str, user: str = Depends(get_current_user)):
    return manager.job_view(_owned(run_id, user))


@router.get("/queue")
def queue(user: str = Depends(get_current_user)):
    return manager.queue_view()


@router.get("/run/{run_id}/events")
async def run_events(run_id: str, request: Request, user: str = Depends(get_current_user)):
    job = _owned(run_id, user)

    async def gen():
        last = None
        while True:
            if await request.is_disconnected():
                break
            snap = manager.job_view(job)
            payload = json.dumps(snap)
            if payload != last:
                yield f"data: {payload}\n\n"
                last = payload
            if snap["status"] in ("done", "error", "stopped"):
                break
            await asyncio.sleep(0.5)

    return StreamingResponse(gen(), media_type="text/event-stream",
                             headers={"Cache-Control": "no-cache",
                                      "X-Accel-Buffering": "no"})
