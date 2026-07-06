#!/usr/bin/env bash
#
# One-shot installer for MewtwoMegaEvo: builds the C++ simulator + NoiseGen and
# sets up the Python (uv) environment for the web app.
#
# Supports: macOS (Apple Silicon / Intel), Ubuntu, and WSL on Windows.
# Armadillo BLAS backend is chosen automatically:
#   macOS         -> Accelerate (built in)
#   Intel x86_64  -> Intel oneMKL (OneAPI)      [override: --blas=openblas]
#   AMD  x86_64   -> OpenBLAS (via conan)
#
# Usage:
#   ./install.sh [options]
#     --blas=auto|mkl|openblas|accelerate   backend (default: auto)
#     --skip-cpp        don't build the C++ binaries
#     --skip-python     don't set up the Python env
#     --jobs=N          parallel build jobs (default: all cores)
#     -y, --yes         non-interactive (assume yes to prompts)
#     -h, --help        this help
#
set -euo pipefail

# ----------------------------------------------------------------------------
BLAS="auto"; SKIP_CPP=0; SKIP_PY=0; JOBS=""; ASSUME_YES=0
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

c()  { printf '\033[%sm%s\033[0m' "$1" "$2"; }
log()  { printf '%s %s\n' "$(c '1;34' '==>')" "$1"; }
ok()   { printf '%s %s\n' "$(c '1;32' ' ok')" "$1"; }
warn() { printf '%s %s\n' "$(c '1;33' ' !!')" "$1" >&2; }
die()  { printf '%s %s\n' "$(c '1;31' 'err')" "$1" >&2; exit 1; }

usage() { sed -n '2,19p' "$0" | sed 's/^# \{0,1\}//'; exit 0; }

for arg in "$@"; do
  case "$arg" in
    --blas=*)   BLAS="${arg#*=}";;
    --skip-cpp) SKIP_CPP=1;;
    --skip-python) SKIP_PY=1;;
    --jobs=*)   JOBS="${arg#*=}";;
    -y|--yes)   ASSUME_YES=1;;
    -h|--help)  usage;;
    *) die "unknown option: $arg (try --help)";;
  esac
done

confirm() {  # confirm "question" -> 0 yes / 1 no
  [ "$ASSUME_YES" = 1 ] && return 0
  printf '%s [y/N] ' "$1"; read -r ans </dev/tty || return 1
  [[ "$ans" =~ ^[Yy] ]]
}
have() { command -v "$1" >/dev/null 2>&1; }

# ----------------------------------------------------------------------------
# Detect platform + CPU
OS="$(uname -s)"; ARCH="$(uname -m)"; IS_WSL=0; PKG=""
CPU_VENDOR="unknown"

if [ "$OS" = "Linux" ]; then
  if [ -n "${WSL_DISTRO_NAME:-}" ] || grep -qiE "microsoft|wsl" /proc/version 2>/dev/null; then
    IS_WSL=1
  fi
  have apt-get && PKG="apt"
  if have lscpu; then
    lscpu | grep -qi "GenuineIntel" && CPU_VENDOR="intel"
    lscpu | grep -qi "AuthenticAMD" && CPU_VENDOR="amd"
  elif [ -r /proc/cpuinfo ]; then
    grep -qi "GenuineIntel" /proc/cpuinfo && CPU_VENDOR="intel"
    grep -qi "AuthenticAMD" /proc/cpuinfo && CPU_VENDOR="amd"
  fi
elif [ "$OS" = "Darwin" ]; then
  PKG="brew"
  if [ "$ARCH" = "arm64" ]; then CPU_VENDOR="apple"; else CPU_VENDOR="intel"; fi
fi

plat="$OS $ARCH"; [ "$IS_WSL" = 1 ] && plat="$plat (WSL)"
log "Platform: $plat  |  CPU: $CPU_VENDOR"

# Resolve BLAS backend
if [ "$BLAS" = "auto" ]; then
  case "$CPU_VENDOR" in
    apple)  BLAS="accelerate";;
    intel)  [ "$OS" = "Darwin" ] && BLAS="accelerate" || BLAS="mkl";;
    amd)    BLAS="openblas";;
    *)      BLAS=$([ "$OS" = "Darwin" ] && echo accelerate || echo openblas);;
  esac
fi
log "Armadillo BLAS backend: $(c '1;36' "$BLAS")"

# ----------------------------------------------------------------------------
# Package-manager helpers
brew_install() { for p in "$@"; do brew list "$p" >/dev/null 2>&1 || brew install "$p"; done; }
apt_install()  { sudo apt-get install -y "$@"; }

ensure_pkg_manager() {
  if [ "$PKG" = "brew" ]; then
    have brew || {
      log "Installing Homebrew…"
      /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
      # add brew to PATH for this shell
      [ -x /opt/homebrew/bin/brew ] && eval "$(/opt/homebrew/bin/brew shellenv)"
      [ -x /usr/local/bin/brew ]    && eval "$(/usr/local/bin/brew shellenv)"
    }
    # Command Line Tools provide clang/make
    have clang || xcode-select --install 2>/dev/null || true
  elif [ "$PKG" = "apt" ]; then
    log "Updating apt package lists…"; sudo apt-get update -y
  else
    die "No supported package manager found (need Homebrew on macOS or apt on Linux)."
  fi
}

install_build_prereqs() {
  log "Installing build prerequisites (compiler, cmake, git)…"
  if [ "$PKG" = "brew" ]; then
    brew_install cmake git pkg-config
  else
    apt_install build-essential cmake git curl pkg-config ca-certificates
  fi
  ok "build tools ready"
}

install_uv() {
  if have uv; then ok "uv already installed ($(uv --version))"; return; fi
  log "Installing uv…"
  curl -LsSf https://astral.sh/uv/install.sh | sh
  export PATH="$HOME/.local/bin:$HOME/.cargo/bin:$PATH"
  have uv || die "uv installed but not on PATH; add ~/.local/bin to PATH and re-run."
  ok "uv installed ($(uv --version))"
}

install_conan() {
  if have conan; then ok "conan already installed ($(conan --version))"; return; fi
  log "Installing conan (via uv tool)…"
  uv tool install conan
  export PATH="$HOME/.local/bin:$PATH"
  have conan || die "conan installed but not on PATH; add ~/.local/bin to PATH and re-run."
  ok "conan installed ($(conan --version))"
}

# ----------------------------------------------------------------------------
# Intel MKL (OneAPI) on Linux
MKL_ENV=""
setup_mkl() {
  # already available?
  if [ -n "${MKLROOT:-}" ] && [ -d "${MKLROOT}" ]; then
    ok "MKL found at $MKLROOT"; return
  fi
  local setvars=/opt/intel/oneapi/setvars.sh
  if [ -f "$setvars" ]; then
    MKL_ENV="$setvars"; return
  fi
  if [ "$PKG" != "apt" ]; then
    warn "MKL requested but auto-install only supported via apt; falling back to OpenBLAS."
    BLAS="openblas"; return
  fi
  if ! confirm "Install Intel oneMKL (OneAPI) via apt? (large download)"; then
    warn "Skipping MKL; using OpenBLAS instead."; BLAS="openblas"; return
  fi
  log "Adding Intel OneAPI apt repository…"
  wget -qO- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
    | gpg --dearmor | sudo tee /usr/share/keyrings/oneapi-archive-keyring.gpg >/dev/null
  echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
    | sudo tee /etc/apt/sources.list.d/oneAPI.list >/dev/null
  sudo apt-get update -y
  apt_install intel-oneapi-mkl intel-oneapi-mkl-devel
  MKL_ENV=/opt/intel/oneapi/setvars.sh
  [ -f "$MKL_ENV" ] || { warn "MKL setvars not found after install; using OpenBLAS."; BLAS="openblas"; }
}

# ----------------------------------------------------------------------------
build_cpp() {
  log "Building C++ (MewtwoMegaEvo + NoiseGen) with backend '$BLAS'…"
  cd "$REPO_DIR"
  export MEWTWO_BLAS="$BLAS"
  if [ "$BLAS" = "mkl" ] && [ -n "$MKL_ENV" ]; then
    log "Sourcing $MKL_ENV"; set +u; source "$MKL_ENV"; set -u
  fi
  local jobs="${JOBS:-$( (nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) )}"

  conan profile detect --force >/dev/null 2>&1 || conan profile detect --force
  log "conan install (this fetches/builds dependencies)…"
  conan install . --output-folder=build --build=missing -s build_type=Release
  local toolchain="$REPO_DIR/build/conan_toolchain.cmake"
  [ -f "$toolchain" ] || toolchain="$REPO_DIR/build/build/Release/generators/conan_toolchain.cmake"
  [ -f "$toolchain" ] || die "conan_toolchain.cmake not found under build/ — conan install may have failed."

  log "cmake configure + build (-j$jobs)…"
  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j"$jobs"

  [ -x "$REPO_DIR/Playground/MewtwoMegaEvo" ] || die "Build finished but Playground/MewtwoMegaEvo is missing."
  ok "C++ binaries built → Playground/MewtwoMegaEvo, Playground/NoiseGen"
}

setup_python() {
  log "Setting up Python environment (uv sync)…"
  cd "$REPO_DIR"
  uv sync
  ok "Python env ready (.venv with web app + mewtwo dataloader)"
}

# ----------------------------------------------------------------------------
ensure_pkg_manager
[ "$SKIP_CPP" = 0 ] && { install_build_prereqs; install_conan; }
install_uv
[ "$BLAS" = "mkl" ] && setup_mkl
[ "$SKIP_CPP" = 0 ] && build_cpp || warn "Skipping C++ build (--skip-cpp)."
[ "$SKIP_PY"  = 0 ] && setup_python || warn "Skipping Python setup (--skip-python)."

echo
ok "$(c '1;32' 'Done!')"
echo "Run the web app:"
echo "  $(c 1 'uv run uvicorn webapp.main:app --host 0.0.0.0 --port 8000')"
echo "Then open http://<this-host>:8000  (the server prints its LAN IP on startup)."
