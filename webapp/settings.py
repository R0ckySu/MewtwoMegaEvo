"""Central configuration for the MewtwoMegaEvo web app.

Paths can be set three ways, in priority order:
  1. environment variable  (e.g. MEWTWO_USERDATA)
  2. a YAML config file     (default: <repo>/server_config.yaml, or MEWTWO_CONFIG)
  3. built-in default
See server_config.example.yaml for the available keys.
"""
import os
import secrets
import sys
from pathlib import Path

try:
    import yaml
except ImportError:  # pyyaml is a declared dependency, but degrade gracefully
    yaml = None

# webapp/ -> project root (the MewtwoMegaEvo repo)
WEBAPP_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = WEBAPP_DIR.parent

# ---- load the optional YAML server config --------------------------------
CONFIG_FILE = Path(os.environ.get("MEWTWO_CONFIG",
                                  PROJECT_ROOT / "server_config.yaml")).expanduser()
_cfg: dict = {}
if yaml is not None and CONFIG_FILE.is_file():
    try:
        _cfg = yaml.safe_load(CONFIG_FILE.read_text()) or {}
    except Exception:
        _cfg = {}


def _path(env_name: str, yaml_key: str, default: Path) -> Path:
    val = os.environ.get(env_name)
    if val:
        return Path(val).expanduser().resolve()
    if _cfg.get(yaml_key):
        return Path(str(_cfg[yaml_key])).expanduser().resolve()
    return default


def _value(env_name: str, yaml_key: str, default):
    val = os.environ.get(env_name)
    if val is not None:
        return val
    if _cfg.get(yaml_key) is not None:
        return _cfg[yaml_key]
    return default


# Directory that holds the compiled binary (subprocess cwd; relative noise
# waveform paths like "./NoiseData/..." resolve against it).
PLAYGROUND_DIR = _path("MEWTWO_PLAYGROUND", "playground_dir", PROJECT_ROOT / "Playground")

# The compiled simulator + noise binaries.
BINARY_PATH = _path("MEWTWO_BINARY", "binary_path", PLAYGROUND_DIR / "MewtwoMegaEvo")
NOISEGEN_BINARY = _path("MEWTWO_NOISEGEN", "noisegen_binary", PLAYGROUND_DIR / "NoiseGen")

# Shared, large noise-data store (shared by all users).
NOISEDATA_DIR = _path("MEWTWO_NOISEDATA", "noise_data_dir", PLAYGROUND_DIR / "NoiseData")

# Demo config templates.
DEMO_CONFIGS_DIR = _path("MEWTWO_DEMOS", "demos_dir", PLAYGROUND_DIR / "Demo_configs")

# Per-user data: userdata/<user>/{config_files, <task><timestamp>}
USERDATA_DIR = _path("MEWTWO_USERDATA", "user_data_dir", WEBAPP_DIR / "userdata")

# SQLite user store.
USERS_DB = _path("MEWTWO_USERS_DB", "users_db", WEBAPP_DIR / "users.db")

# Python interpreter that has the `mewtwo` package (for plotting). Defaults to
# the app's own interpreter since mewtwo lives in the same uv env.
MEWTWO_PY = _path("MEWTWO_PY", "mewtwo_py", Path(sys.executable))
PLOT_EXTRACTOR = WEBAPP_DIR / "plot_extract.py"

# The three config filenames the binary requires.
CONFIG_FILES = ("sim_config.json", "gate_config.json", "hamiltonian_config.json")

# Make sure the configured locations exist.
for _d in (USERDATA_DIR, NOISEDATA_DIR, USERS_DB.parent):
    try:
        _d.mkdir(parents=True, exist_ok=True)
    except OSError:
        pass


def _session_secret() -> str:
    """Cookie-signing secret: MEWTWO_SECRET / YAML `secret`, else a random
    secret persisted to userdata/.session_secret (strong + stable, no setup)."""
    explicit = _value("MEWTWO_SECRET", "secret", None)
    if explicit:
        return str(explicit)
    secret_file = USERDATA_DIR / ".session_secret"
    try:
        if secret_file.is_file():
            existing = secret_file.read_text().strip()
            if existing:
                return existing
        generated = secrets.token_hex(32)
        secret_file.write_text(generated)
        try:
            secret_file.chmod(0o600)
        except OSError:
            pass
        return generated
    except OSError:
        return "dev-insecure-change-me"


SESSION_SECRET = _session_secret()
SESSION_SECRET_SOURCE = (
    "explicit (env/yaml)" if _value("MEWTWO_SECRET", "secret", None)
    else "auto-generated (userdata/.session_secret)")
CONFIG_FILE_LOADED = CONFIG_FILE if _cfg else None
