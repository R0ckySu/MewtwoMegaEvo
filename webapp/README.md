# MewtwoMegaEvo Web Interface

A browser UI for configuring and running the MewtwoMegaEvo simulator. It edits the
three JSON configs with type-aware forms, lets you browse/edit the referenced
matrix/vector files, launches the compiled binary, and streams a live progress bar.

It replaces an earlier Wt-based C++ GUI (now removed). It is fully decoupled from the C++
code: it drives the already-compiled `Playground/MewtwoMegaEvo` binary as a subprocess.
**No C++ changes are required.**

## Stack

- **Backend:** FastAPI + uvicorn (Python), managed with **uv**. Session-cookie auth, SQLite user store.
- **Frontend:** zero-build single-page app (Vue 3 from CDN) served by FastAPI. No Node.
- **Progress:** parsed from the binary's stdout `Solver job done! [N/M]` lines and pushed
  to the browser via Server-Sent Events.

## Project layout

```
<repo root>/
  pyproject.toml       # single uv project: web app + mewtwo dataloader, one env
  uv.lock
  webapp/              # the FastAPI app (this package)
  mewtwo/              # HDF5 dataloader / post-processing package (importable, reusable)
  Playground/          # the compiled binary, Demo_configs/, NoiseData/, ...
```

## Setup

Easiest is the repo-root installer, which also builds the C++ binaries:

```bash
./install.sh                 # everything; or `./install.sh --skip-cpp` for just the web env
```

Or set up only the Python side by hand:

```bash
cd /Users/rockysu/CodeRepo/MewtwoMegaEvo.git
uv sync          # creates ./.venv with the web app AND the mewtwo dataloader (Python 3.10+)
```

Both `webapp` and `mewtwo` are installed into one environment, so plotting loads HDF5 results
with `mewtwo` from the same interpreter — no separate env to manage. (`import mewtwo` also works
in notebooks via `uv run python` or by activating `.venv`.)

## Run

```bash
uv run uvicorn webapp.main:app --host 127.0.0.1 --port 8000
```

### Sharing with other users on your LAN

Just bind to all interfaces:

```bash
uv run uvicorn webapp.main:app --host 0.0.0.0 --port 8000
```

Others then open `http://<this-machine-LAN-IP>:8000` (the server prints the LAN IP on startup).
**Sign-in goes through the lab portal (LabPortal)** — set `MEWTWO_PORTAL` (or `portal_url` in the
config YAML) to the portal base URL; there is no local registration or password login. A browser
holding a valid portal session with Mewtwo access is signed in automatically and gets an isolated
data sandbox keyed by the portal username; noise data is shared.

The cookie-signing **secret is managed for you**: a strong random secret is generated on first run
and saved to `userdata/.session_secret`, so logins stay valid across restarts. To use a shared
secret across several server hosts (or supply your own), set `MEWTWO_SECRET`:

```bash
MEWTWO_SECRET="$(python3 -c 'import secrets;print(secrets.token_hex(32))')" \
  uv run uvicorn webapp.main:app --host 0.0.0.0 --port 8000
```

Make sure your firewall allows inbound TCP on the port. It serves plain HTTP, which is fine on a
trusted lab network; don't expose it to the public internet as-is. (Changing the secret invalidates
existing cookies — everyone just logs in again.)

Open http://127.0.0.1:8000 — you'll be bounced through the portal sign-in. Each user gets one
Playground-like sandbox on disk:

```
webapp/userdata/<user>/config_files/        # the single editable config
webapp/userdata/<user>/<task><timestamp>/   # each run's output (directly here)
```

## Workflow

1. **Editor** – edit your single config. **Load demo…** (top bar) replaces it with one of the
   `Playground/Demo_configs` templates (configs + matrix/vector files); **Reset** clears it.
2. **Sim / Gates / Hamiltonians** tabs – type-aware forms. The Hamiltonian `type`
   dropdown (`static` / `static_RF` / `mw` / `mw_RF` / `awg` / `noise`) renders exactly
   that type's fields. Add multiple sweep-parameter rows in the Sim tab.
   - Gates and Hamiltonians render as **cards in a waterfall/masonry layout**, each with a
     subtle colour accent per type (gate: `switch`/`shaped`/`sticky`; Hamiltonian: the six
     types). A gate's Hamiltonians field is a chip-list with autocomplete over the
     Hamiltonians you've defined.
   - Within each card, fields are ordered **to match your JSON file's key order**.

### Noise (shared)

The **Noise** view generates and browses noise data via the `NoiseGen` binary. Because the
data is large, it is **shared by all users** in one global store (`Playground/NoiseData`),
not per user.

- **Generate** — a `noise_config` form (tag, mode `colored`/`arb`, channels, start index,
  time step, length, amplitude, plus `alpha` for colored or a spectrum `noise_expr` for arb).
  Shows a rough size estimate and a live progress bar (channels written, parsed from NoiseGen's
  output). Runs with cwd = Playground so `./NoiseData/...` resolves.
- **Cached noise groups** — a table with `time_step`, `channels`, `length`, mode and size.
  **Click a group's name** to copy its `waveform_path` (in the exact form a noise Hamiltonian
  expects, with `#` as the channel placeholder) to your clipboard; **click elsewhere in the row**
  to see the config it was generated with; **Delete** removes a group from the shared store.

The top bar has three views: **Editor**, **Noise** (above), and **Projects** — a portal
listing your previous runs (the `<task><timestamp>` folders under your user directory). From
the portal you can:

- **Plot** (📈) — browse `meas_marker` data (see below).
- **Download** (⬇) a run as a zip.
- **Load config** (📂) — replaces your editor config with the one that run used
  (auto-migrated), then drops you into the editor.
- **Delete** (🗑) a run (removes its results and saved config permanently).

Actions are compact icon buttons. A **☰ / ▦ toggle** switches between the **list** and a
**gallery** of each run's saved plot thumbnails. When two neighbouring runs share a task
name, a short **change note** shows what differs in the config (e.g.
`repeat: 1 → 4`, `record_density_mat: off → on`, or a changed sweep range).

### Plots (meas_marker)

An interactive Plotly browser for `meas_marker` expectation values, available both from
**Projects → Plot** and inline on the **Editor → Run** tab (it appears automatically when a
run finishes). The mewtwo loader reconstructs the N-D sweep grid — including multi-parameter
runs where params were meshgrid-spanned into `<name>_span` files, which `param_fold` reverts
back to labelled axes — and caches a NetCDF next to the result for fast re-opening. Controls:

- **Observable** and **Init state** — separate dropdowns selecting the `meas_marker_<obs>_<init>`
  variable.
- **Line** or **Heatmap** — 1-D trace vs a chosen axis, or a 2-D map (e.g. sweep × marker,
  i.e. a Rabi chevron).
- **X / Y axis** — any dimension (a swept parameter, `marker_index`, or `marker_repeat`).
- **X / Y scale** — linear or log per axis.
- **Sliders** fix the remaining dimensions (showing the coordinate value for swept params),
  so for d>2 runs you choose which 2 dimensions to plot and slice the rest.
- **Save plot** — writes a PNG into the run's `plots/` folder; the default name embeds the
  sliced coordinates (e.g. `Z_Z_line_pw_span=5e-06.png`) so different slices don't collide.
  Saved plots show as thumbnails below and are browsable on any later visit.

If the run had **Record density matrix** enabled, the plot area shows a
**Measurements / Density matrix** toggle. The density view puts a **marker slider**
(and a param slider for swept runs) over a table of the ρ matrix at that marker
(complex entries, 3 significant figures). **Click matrix cells** to plot each
element's trace on the right; the selected cells are tinted with the same colours as
their lines, and a Re / Im / Magnitude selector chooses what the traces show. The
trace **x-axis** can be **marker index** or the **swept parameter** (element vs
parameter at the fixed marker). **Hover a cell** for the value at 9 significant
figures (the table shows 3). **Save plot** writes a composite PNG — the selected
matrix, the marker/param slider state, and the traces — into the run's `plots/`
folder. (Density matrices are read straight from the HDF5 `rho_marker` group.)

The Run tab also shows a **live** panel while a simulation is running. Note the current binary
writes its HDF5 without SWMR / incremental flush, so completed points aren't readable until the
run finishes — in practice the live panel shows "waiting…" and the full plot appears the moment
the run completes. (The live path already streams if a future build flushes per point.)
   Any file-reference field (matrix symbols like `h_pauli_mat`, sweep parameter files,
   observable/state symbols) shows a **⤢ preview** button / is clickable — it opens a popup
   that renders the matrix as a grid (or the vector as text) without leaving the form. In the
   sweep table the numeric/string **value type** is an inline control on the parameter file
   itself (editable only for Hamiltonian sweeps; Gate is always numeric, Sequence always string).
3. **Symbols** tab (the former Files tab) – browse and edit the referenced
   matrix/vector/parameter files. Files referenced by a config are tagged `ref`;
   referenced-but-absent files are tagged `missing`. Two generators sit at the top:
   - **Create vector** – write a numerical vector file (one value per line) with
     evenly spaced (`linspace`) or geometrically spaced (`logspace`) values, ready to
     use as a sweep-parameter file.
   - **Span into mesh grid** – pick two or more vector files in order (first varies
     fastest) and tensor them into a flattened N-D meshgrid; each input `<name>` gets a
     `<name>_span` file of length = product of the input lengths (via `mewtwo.param_span`,
     the exact form `param_fold` reads back). Point one sweep-parameter row at each
     `_span` file to scan several parameters together.
4. **Run** tab – *Save all* then *Launch*. Watch the progress bar; on completion, download
   the result folder as a zip. Past results are listed below. A **Host resources** dashboard
   at the top shows, graphically, live **CPU-average and memory donut gauges**, a **per-core
   CPU** bar grid (scales to many-core machines), and a memory/swap breakdown with pressure —
   polled every 1.5 s while on the tab. Per-core CPU is sampled by a background thread (1 s
   window) so requests never block; memory pressure uses Linux PSI (`/proc/pressure/memory`)
   when available.

Runs are **restart-safe**: the simulator is launched in its own session with stdout going to
a log file (not a pipe), so a server restart/crash cannot kill an in-progress run — it keeps
running, finishes writing its results, and the server just tails the log for progress. User
data lives in `webapp/userdata/<user>/` (override with `MEWTWO_USERDATA`); nothing in the app
ever deletes a whole user folder.

### Job queue (single concurrent job, server-wide)

Only **one simulation runs at a time** across the whole server; further submissions **queue**
(FIFO). The **Job queue** card on the Run tab shows the running job and everyone's queued jobs
with positions and an **ETA**. Your own job's status card shows your queue position and estimated
start time, then switches to a progress bar + estimated time remaining when it starts.

- Each job **snapshots your config at submit time**, so editing afterwards doesn't affect a
  queued job.
- **Launch** enqueues; **Cancel** removes a queued job, **Stop** terminates a running one.
- ETA is learned online: a moving average of seconds-per-work-unit (`num_params × repeat`) from
  completed jobs, so estimates sharpen after the first run finishes.

## Legacy demo migration

The demos in `Playground/Demo_configs/` predate the current binary and omit fields
its C++ constructors now read unconditionally (e.g. a gate's `type`, or a noise
Hamiltonian's `rand_shift` / `num_available_channels`). Loading such a config makes the
binary throw `nlohmann type_error ... is null` before any solver step.

`migrate.py` fixes this **non-destructively** — it fills the required fields with safe
defaults derived directly from the constructors (`Gate`, `*_Hamiltonian`):

- **Load demo** / **Load config** (from a past run) auto-migrates the configs and reports
  what changed and any remaining blockers.
- The **"Migrate legacy configs"** button (Sim tab) migrates your current config on demand
  and reloads the forms.

It also flags blockers it can't fix — most commonly an `enable`d `noise` Hamiltonian whose
`NoiseData/` directory is absent (those files are gitignored). Provide the data or disable
that Hamiltonian. After migration, noise-free demos (RabiChevron, CNOT, …) run end-to-end.

## Configuration

Set locations either in a **YAML config file** or via **environment variables** (env wins, then
YAML, then the default). Copy `server_config.example.yaml` to `server_config.yaml` in the repo
root (or point `MEWTWO_CONFIG` at any path) and fill in what you need:

```yaml
user_data_dir:  /srv/mewtwo/userdata    # per-user configs + run outputs
noise_data_dir: /srv/mewtwo/NoiseData   # shared cached noise data
users_db:       /srv/mewtwo/users.db    # SQLite user store
# secret: "..."   # optional shared cookie secret (else auto-generated)
```

| Env var / YAML key                | Default                          | Meaning                                  |
|-----------------------------------|----------------------------------|------------------------------------------|
| `MEWTWO_USERDATA` / `user_data_dir`  | `webapp/userdata/`            | Per-user config + run outputs.           |
| `MEWTWO_NOISEDATA` / `noise_data_dir`| `Playground/NoiseData/`       | Shared noise-data store.                 |
| `MEWTWO_USERS_DB` / `users_db`       | `webapp/users.db`             | SQLite user store.                       |
| `MEWTWO_SECRET` / `secret`           | auto-gen → `userdata/.session_secret` | Cookie-signing secret.           |
| `MEWTWO_BINARY` / `binary_path`      | `Playground/MewtwoMegaEvo`    | The simulator executable.                |
| `MEWTWO_PLAYGROUND` / `playground_dir`| `Playground/`                | Subprocess cwd (so `./NoiseData/...` resolves). |
| `MEWTWO_NOISEGEN` / `noisegen_binary`| `Playground/NoiseGen`         | The noise generator binary.              |
| `MEWTWO_DEMOS` / `demos_dir`         | `Playground/Demo_configs/`    | Demo templates ("Load demo").            |
| `MEWTWO_PY` / `mewtwo_py`            | the app's own interpreter     | Interpreter with `mewtwo` (for plots).   |
| `MEWTWO_CONFIG`                      | `server_config.yaml`          | Path to the YAML config file itself.     |
| `MEWTWO_PORTAL` / `portal_url`       | *(unset → sign-in disabled)*  | Lab portal base URL; the only sign-in path. |
| `MEWTWO_SMTP_HOST` / `smtp_host`     | *(unset → email off)*         | SMTP server for long-run email reports.  |
| `MEWTWO_SMTP_PORT` / `smtp_port`     | `587`                         | SMTP port (465 for implicit TLS).        |
| `MEWTWO_SMTP_USER` / `smtp_user`     | *(none)*                      | SMTP login user.                         |
| `MEWTWO_SMTP_PASSWORD` / `smtp_password` | *(none)*                  | SMTP password / app password.            |
| `MEWTWO_MAIL_FROM` / `mail_from`     | = `smtp_user`                 | From address for reports.                |
| `MEWTWO_SMTP_STARTTLS` / `smtp_starttls` | `true`                    | Use STARTTLS (port 587).                 |
| `MEWTWO_SMTP_SSL` / `smtp_ssl`       | `false`                       | Implicit TLS (port 465).                 |
| `MEWTWO_EMAIL_MIN_SECONDS` / `email_min_seconds` | `300`             | Only email for runs at least this long.  |

**Registration now requires an email address.** If SMTP is configured (above), a
user gets a short report by email whenever one of their jobs runs longer than
`email_min_seconds` (default 5 min) — with status, duration, work units, the result
folder, and (on failure) the last log lines. Email is entirely optional: leave
`smtp_host` unset and nothing is sent (addresses are still collected).

**Plot defaults:** a result with two or more swept dimensions opens as a 2-D
**heatmap** first. Switch **Type → Line** and the second dimension becomes a set of
**legend traces** (many lines on one plot); pick which dimension via the **Series**
control, or set it to *(single line)* to slice with a slider instead. Large series
are subsampled evenly so the legend stays readable.

The noise store honours whichever `noise_data_dir` you set — generation writes there and the
copied `waveform_path` is relative (`./NoiseData/...`) when it lives under Playground, else an
absolute path so the simulator finds it regardless of cwd.

The simulator is invoked as:
`MewtwoMegaEvo -c <user>/config_files -o <user> -t <timestamp>` (cwd = Playground), so a run
produces `userdata/<user>/<task><timestamp>/`.

## Notes

- Progress relies on the binary being built with `-D_TASK_PROGRESS_` (it is, per
  `QSimTask/CMakeLists.txt`) and on `sim_config.log_level` being high enough that the
  `Solver job done! [N/M]` line prints (the demo default `4` works).
- The file editor loads files up to 2 MB; larger files open read-only as a preview.
