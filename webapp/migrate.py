"""Migrate legacy configs to the field set the current binary requires.

The demos in Playground/Demo_configs predate the current binary and omit fields
that the C++ constructors now read unconditionally (Gate / *_Hamiltonian in
QSimTask/QSimCoreLib). Missing/null values make nlohmann::json throw
`type_error.302 ... is null` at load time, before any solver iteration.

This module fills the required fields with safe defaults (non-destructive: it
never removes or disables anything) and reports any remaining blockers it can
detect, such as referenced data files that are absent on disk.

Defaults are derived directly from the constructors:
  Gate::Gate                -> type, shift_time, hamiltonians, ext_shaped_sig_path, pulse_width
  Hamiltonian::Hamiltonian  -> amplitude, h_pauli_mat, waveform_path (+ enable, read in dispatch)
  Gated_Hamiltonian         -> rising_time, falling_time
  MW / MW_RF                -> freq, phase, chirp_rate  (MW_RF also RF_freq_mat, wave_forward_propagate)
  Static_RF                 -> RF_freq_mat
  Noise_Hamiltonian         -> lag_time, rand_shift, rand_channel, num_available_channels
"""
import json
from typing import Any, Dict, List

from . import schema, settings

# Canonical key order for a gate = the schema field order (tag, type, ...).
_GATE_ORDER = [f["name"] for f in schema.GATE_FIELDS]


def _ham_order(htype: str) -> List[str]:
    """Canonical key order for a Hamiltonian: tag, type, then the type's fields."""
    names = [f["name"] for f in schema.HAMILTONIAN_TYPES.get(htype, [])]
    return ["tag", "type"] + [n for n in names if n != "tag"]


def _reorder(obj: Dict[str, Any], order: List[str]) -> Dict[str, Any]:
    """Return obj with keys in `order` first (those present), extras appended."""
    new = {k: obj[k] for k in order if k in obj}
    for k, v in obj.items():
        if k not in new:
            new[k] = v
    return new

GATE_DEFAULTS: Dict[str, Any] = {
    "type": "switch", "shift_time": 0, "hamiltonians": [],
    "ext_shaped_sig_path": "", "pulse_width": 0,
}

_H_COMMON = {"enable": True, "amplitude": 0, "h_pauli_mat": "", "waveform_path": ""}
_H_GATED = {"rising_time": 0, "falling_time": 0}
_H_MW = {**_H_GATED, "freq": 0, "phase": 0, "chirp_rate": 0}

HAMILTONIAN_DEFAULTS: Dict[str, Dict[str, Any]] = {
    "static": {**_H_COMMON},
    "static_RF": {**_H_COMMON, "RF_freq_mat": ""},
    "mw": {**_H_COMMON, **_H_MW},
    "mw_RF": {**_H_COMMON, **_H_MW, "RF_freq_mat": "", "wave_forward_propagate": False},
    "awg": {**_H_COMMON, **_H_GATED},
    "noise": {**_H_COMMON, "lag_time": 0, "rand_shift": False, "rand_channel": False,
              "num_available_channels": 1},
}

SIM_DEFAULTS: Dict[str, Any] = {
    "log_level": 4, "job_slicing_strategy": "linspace", "record_propagator": False,
    "record_all_meas": False, "record_density_mat": True, "reset_rotating_frame": False,
    "enable_param_parallel_mode": True, "repeat": 1, "sweep_param_info": [],
}


def _fill(obj: Dict[str, Any], defaults: Dict[str, Any], label: str,
          changes: List[str]) -> None:
    for key, default in defaults.items():
        if key not in obj or obj[key] is None:
            obj[key] = default
            changes.append(f"{label}: added '{key}' = {json.dumps(default)}")


def migrate_sim(data: Dict[str, Any], changes: List[str]) -> Dict[str, Any]:
    _fill(data, SIM_DEFAULTS, "sim_config", changes)
    return data


def migrate_gate(data: Dict[str, Any], changes: List[str]) -> Dict[str, Any]:
    gates = data.get("gate_defs", []) or []
    for i, g in enumerate(gates):
        if isinstance(g, dict):
            _fill(g, GATE_DEFAULTS, f"gate '{g.get('tag', i)}'", changes)
            gates[i] = _reorder(g, _GATE_ORDER)   # keep 'type' after 'tag', etc.
    return data


def migrate_hamiltonian(data: Dict[str, Any], changes: List[str]) -> Dict[str, Any]:
    hams = data.get("hamiltonian_prototype_defs", []) or []
    for i, h in enumerate(hams):
        if not isinstance(h, dict):
            continue
        htype = h.get("type")
        defaults = HAMILTONIAN_DEFAULTS.get(htype)
        if defaults is None:
            changes.append(
                f"hamiltonian '{h.get('tag', i)}': unknown type {htype!r}, left as-is")
            continue
        _fill(h, defaults, f"hamiltonian '{h.get('tag', i)}'", changes)
        hams[i] = _reorder(h, _ham_order(htype))
    return data


def _detect_warnings(cfg_dir) -> List[str]:
    """Blockers a run would still hit even after field-filling."""
    warnings: List[str] = []
    ham_path = cfg_dir / "hamiltonian_config.json"
    if ham_path.is_file():
        try:
            ham = json.loads(ham_path.read_text())
        except json.JSONDecodeError:
            ham = {}
        for h in ham.get("hamiltonian_prototype_defs", []) or []:
            if h.get("type") == "noise" and h.get("enable") and h.get("waveform_path"):
                rel = str(h["waveform_path"]).lstrip("./")
                parent = (settings.PLAYGROUND_DIR / rel).parent
                if not parent.is_dir():
                    warnings.append(
                        f"noise '{h.get('tag')}' needs data dir '{parent}', which is "
                        f"missing — provide the NoiseData files or disable this Hamiltonian.")
    return warnings


def migrate_config_dir(cfg_dir) -> Dict[str, Any]:
    """Migrate the three configs in a config_files dir in place. Returns a report."""
    changes: List[str] = []
    files = {"sim": ("sim_config.json", migrate_sim),
             "gate": ("gate_config.json", migrate_gate),
             "hamiltonian": ("hamiltonian_config.json", migrate_hamiltonian)}
    for _, (fname, fn) in files.items():
        p = cfg_dir / fname
        if not p.is_file():
            continue
        try:
            data = json.loads(p.read_text())
        except json.JSONDecodeError as e:
            changes.append(f"{fname}: could not parse ({e}); skipped")
            continue
        # Order-sensitive so field reordering (e.g. moving 'type' after 'tag')
        # is persisted, not just field additions.
        before = json.dumps(data)
        data = fn(data, changes)
        if json.dumps(data) != before:
            p.write_text(json.dumps(data, indent=2))
    return {"changed": bool(changes), "changes": changes,
            "warnings": _detect_warnings(cfg_dir)}
