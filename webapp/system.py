"""Live host stats for the Run-page dashboard: per-core CPU and memory.

A background thread samples per-core CPU over a 1 s window so requests never
block (important on many-core machines), and the endpoint returns the latest
snapshot plus memory / swap / pressure.
"""
import platform
import threading
from functools import lru_cache

import psutil
from fastapi import APIRouter, Depends

from .auth import get_current_user

router = APIRouter()


class _CpuSampler:
    def __init__(self):
        self._per_core = []
        self._lock = threading.Lock()
        self._started = False

    def start(self):
        if self._started:
            return
        self._started = True
        psutil.cpu_percent(percpu=True)  # prime the first delta
        threading.Thread(target=self._loop, daemon=True).start()

    def _loop(self):
        while True:
            vals = psutil.cpu_percent(percpu=True, interval=1.0)
            with self._lock:
                self._per_core = [round(v, 1) for v in vals]

    def per_core(self):
        with self._lock:
            return list(self._per_core)


_sampler = _CpuSampler()


@lru_cache(maxsize=1)
def _host_info():
    model = platform.processor() or platform.machine()
    if platform.system() == "Linux":
        try:
            with open("/proc/cpuinfo") as f:
                for line in f:
                    if line.startswith("model name"):
                        model = line.split(":", 1)[1].strip()
                        break
        except OSError:
            pass
    return {"cpu_model": model, "cores_logical": psutil.cpu_count(),
            "cores_physical": psutil.cpu_count(logical=False),
            "platform": platform.platform()}


def _mem_pressure():
    """Linux PSI (some-avg over 10/60 s); None elsewhere."""
    if platform.system() != "Linux":
        return None
    try:
        with open("/proc/pressure/memory") as f:
            for line in f:
                if line.startswith("some"):
                    parts = dict(p.split("=") for p in line.split()[1:] if "=" in p)
                    return {"some_avg10": float(parts.get("avg10", 0)),
                            "some_avg60": float(parts.get("avg60", 0))}
    except (OSError, ValueError):
        pass
    return None


@router.get("/system")
def system_stats(user: str = Depends(get_current_user)):
    _sampler.start()
    per = _sampler.per_core()
    vm = psutil.virtual_memory()
    sw = psutil.swap_memory()
    try:
        load = [round(x, 2) for x in psutil.getloadavg()]
    except (OSError, AttributeError):
        load = None
    overall = round(sum(per) / len(per), 1) if per else None
    return {
        "cpu": {"per_core": per, "count": len(per) or psutil.cpu_count(),
                "overall": overall, "loadavg": load},
        "mem": {"total": vm.total, "used": vm.used,
                "available": vm.available, "percent": vm.percent},
        "swap": {"total": sw.total, "used": sw.used, "percent": sw.percent},
        "pressure": _mem_pressure(),
        "host": _host_info(),
    }
