"""Central configuration for the MewtwoMegaEvo web app.

All paths are derived from the repository layout but can be overridden with
environment variables so the app is portable across machines.
"""
import os
import sys
from pathlib import Path

# webapp/ -> project root (the MewtwoMegaEvo repo)
WEBAPP_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = WEBAPP_DIR.parent


def _env_path(name: str, default: Path) -> Path:
    val = os.environ.get(name)
    return Path(val).expanduser().resolve() if val else default


# Directory that holds the compiled binary and NoiseData/ (relative noise
# waveform paths like "./NoiseData/..." resolve against this cwd).
PLAYGROUND_DIR = _env_path("MEWTWO_PLAYGROUND", PROJECT_ROOT / "Playground")

# The compiled simulator binary.
BINARY_PATH = _env_path("MEWTWO_BINARY", PLAYGROUND_DIR / "MewtwoMegaEvo")

# The noise generator binary and the shared noise-data store (shared by all
# users because the generated data is large).
NOISEGEN_BINARY = _env_path("MEWTWO_NOISEGEN", PLAYGROUND_DIR / "NoiseGen")
NOISEDATA_DIR = _env_path("MEWTWO_NOISEDATA", PLAYGROUND_DIR / "NoiseData")

# Where template configs are copied from when creating a workspace.
DEMO_CONFIGS_DIR = _env_path("MEWTWO_DEMOS", PLAYGROUND_DIR / "Demo_configs")

# Per-user data lives here: userdata/<user>/{config_files, <task><timestamp>}
USERDATA_DIR = _env_path("MEWTWO_USERDATA", WEBAPP_DIR / "userdata")

# Python interpreter used to load HDF5 results for plotting. The `mewtwo`
# package lives in this same uv environment, so by default we reuse the app's
# own interpreter (override with MEWTWO_PY if you keep it in a separate env).
MEWTWO_PY = _env_path("MEWTWO_PY", Path(sys.executable))
PLOT_EXTRACTOR = WEBAPP_DIR / "plot_extract.py"

# SQLite user store.
USERS_DB = _env_path("MEWTWO_USERS_DB", WEBAPP_DIR / "users.db")

# Secret used to sign session cookies. Set MEWTWO_SECRET in production.
SESSION_SECRET = os.environ.get("MEWTWO_SECRET", "dev-insecure-change-me")

# The three config filenames the binary requires.
CONFIG_FILES = ("sim_config.json", "gate_config.json", "hamiltonian_config.json")

USERDATA_DIR.mkdir(parents=True, exist_ok=True)
