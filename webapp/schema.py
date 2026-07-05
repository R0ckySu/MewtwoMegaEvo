"""Single source of truth for the config field schemas.

The same schema drives (a) the type-aware forms in the frontend, served via
GET /api/schema, and (b) server-side validation/coercion on save.

Field descriptor keys:
  name     - JSON key
  label    - human label for the UI
  type     - one of: string, int, float, bool, string_list, enum, file,
             hamiltonian_multiselect
  options  - list of allowed values (for type=enum)
  optional - if True, may be absent (default False)
  help     - optional tooltip / hint
A field with type=="file" references another file in the workspace by name.
"""
from typing import Any, Dict, List


# --------------------------------------------------------------------------
# sim_config.json
# --------------------------------------------------------------------------
SIM_FIELDS: List[Dict[str, Any]] = [
    {"name": "task_name", "label": "Task name", "type": "string"},
    {"name": "log_level", "label": "Log level", "type": "int"},
    {"name": "job_slicing_strategy", "label": "Job slicing", "type": "enum",
     "options": ["linspace", "logspace", "inv_logspace"]},
    {"name": "record_propagator", "label": "Record propagator", "type": "bool"},
    {"name": "record_all_meas", "label": "Record all measurements", "type": "bool"},
    {"name": "record_density_mat", "label": "Record density matrix", "type": "bool"},
    {"name": "reset_rotating_frame", "label": "Reset rotating frame", "type": "bool"},
    {"name": "enable_param_parallel_mode", "label": "Param-parallel mode", "type": "bool"},
    {"name": "system_dim", "label": "System dimension", "type": "int"},
    {"name": "observables", "label": "Observables", "type": "string_list"},
    {"name": "init_states", "label": "Initial states", "type": "string_list"},
    {"name": "repeat", "label": "Repeat", "type": "int"},
    {"name": "step_size", "label": "Step size", "type": "float"},
    {"name": "sequence", "label": "Sequence", "type": "string"},
]

# One row of sim_config.sweep_param_info
SWEEP_CLASSES = ["Sequence", "Gate", "Hamiltonian"]
SWEEP_FIELDS: List[Dict[str, Any]] = [
    {"name": "class", "label": "Class", "type": "enum", "options": SWEEP_CLASSES},
    {"name": "tag", "label": "Tag", "type": "string"},
    {"name": "property", "label": "Property", "type": "string", "optional": True,
     "help": "Field to sweep (Gate/Hamiltonian). Leave empty for Sequence."},
    {"name": "val_file", "label": "Value file", "type": "file", "optional": True,
     "help": "Numeric file (Gate/Hamiltonian sweeps)."},
    {"name": "string_file", "label": "String file", "type": "file", "optional": True,
     "help": "Sequence-list file (Sequence sweeps)."},
]

# --------------------------------------------------------------------------
# gate_config.json  ->  gate_defs[]
# --------------------------------------------------------------------------
GATE_FIELDS: List[Dict[str, Any]] = [
    {"name": "tag", "label": "Tag", "type": "string"},
    {"name": "type", "label": "Type", "type": "enum", "options": ["switch", "shaped", "sticky"]},
    {"name": "hamiltonians", "label": "Hamiltonians", "type": "hamiltonian_multiselect"},
    {"name": "pulse_width", "label": "Pulse width", "type": "float"},
    {"name": "shift_time", "label": "Shift time", "type": "float"},
    {"name": "ext_shaped_sig_path", "label": "Shaped-signal file", "type": "file",
     "optional": True},
]

# --------------------------------------------------------------------------
# hamiltonian_config.json  ->  hamiltonian_prototype_defs[]
# Each prototype is keyed by "type". Below are the fixed fields per type.
# --------------------------------------------------------------------------
_H_BASE = [
    {"name": "tag", "label": "Tag", "type": "string"},
    {"name": "enable", "label": "Enable", "type": "bool"},
    {"name": "amplitude", "label": "Amplitude", "type": "float"},
    {"name": "h_pauli_mat", "label": "Pauli/H matrix", "type": "file"},
    {"name": "waveform_path", "label": "Waveform path", "type": "string", "optional": True},
]
_H_ENVELOPE = [
    {"name": "rising_time", "label": "Rising time", "type": "float", "optional": True},
    {"name": "falling_time", "label": "Falling time", "type": "float", "optional": True},
]
_H_DRIVE = [
    {"name": "freq", "label": "Frequency", "type": "float", "optional": True},
    {"name": "phase", "label": "Phase", "type": "float", "optional": True},
    {"name": "chirp_rate", "label": "Chirp rate", "type": "float", "optional": True},
]

HAMILTONIAN_TYPES: Dict[str, List[Dict[str, Any]]] = {
    "static": [
        {"name": "tag", "label": "Tag", "type": "string"},
        {"name": "enable", "label": "Enable", "type": "bool"},
        {"name": "amplitude", "label": "Amplitude", "type": "float"},
        {"name": "h_pauli_mat", "label": "Pauli/H matrix", "type": "file"},
        {"name": "waveform_path", "label": "Waveform path", "type": "string", "optional": True},
    ],
    "static_RF": [
        {"name": "tag", "label": "Tag", "type": "string"},
        {"name": "enable", "label": "Enable", "type": "bool"},
        {"name": "amplitude", "label": "Amplitude", "type": "float"},
        {"name": "RF_freq_mat", "label": "RF frequency matrix", "type": "file"},
        {"name": "h_pauli_mat", "label": "Pauli/H matrix", "type": "file"},
        {"name": "waveform_path", "label": "Waveform path", "type": "string", "optional": True},
    ],
    "mw": [
        {"name": "tag", "label": "Tag", "type": "string"},
        {"name": "enable", "label": "Enable", "type": "bool"},
        {"name": "amplitude", "label": "Amplitude", "type": "float"},
        *_H_ENVELOPE, *_H_DRIVE,
        {"name": "h_pauli_mat", "label": "Pauli/H matrix", "type": "file"},
        {"name": "waveform_path", "label": "Waveform path", "type": "string", "optional": True},
    ],
    "mw_RF": [
        {"name": "tag", "label": "Tag", "type": "string"},
        {"name": "enable", "label": "Enable", "type": "bool"},
        {"name": "amplitude", "label": "Amplitude", "type": "float"},
        *_H_ENVELOPE, *_H_DRIVE,
        {"name": "wave_forward_propagate", "label": "Wave forward propagate", "type": "bool",
         "optional": True},
        {"name": "RF_freq_mat", "label": "RF frequency matrix", "type": "file"},
        {"name": "h_pauli_mat", "label": "Amplitude matrix", "type": "file"},
        {"name": "waveform_path", "label": "Waveform path", "type": "string", "optional": True},
    ],
    "awg": [
        {"name": "tag", "label": "Tag", "type": "string"},
        {"name": "enable", "label": "Enable", "type": "bool"},
        {"name": "amplitude", "label": "Amplitude", "type": "float"},
        *_H_ENVELOPE,
        {"name": "freq", "label": "Frequency", "type": "float", "optional": True},
        {"name": "phase", "label": "Phase", "type": "float", "optional": True},
        {"name": "h_pauli_mat", "label": "Pauli/H matrix", "type": "file"},
        {"name": "waveform_path", "label": "Waveform path", "type": "string", "optional": True},
    ],
    "noise": [
        {"name": "tag", "label": "Tag", "type": "string"},
        {"name": "enable", "label": "Enable", "type": "bool"},
        {"name": "amplitude", "label": "Amplitude", "type": "float"},
        {"name": "h_pauli_mat", "label": "Pauli/H matrix", "type": "file"},
        {"name": "lag_time", "label": "Lag time", "type": "float", "optional": True},
        {"name": "rand_shift", "label": "Random shift", "type": "bool", "optional": True},
        {"name": "rand_channel", "label": "Random channel", "type": "bool", "optional": True},
        {"name": "num_available_channels", "label": "Num channels", "type": "int",
         "optional": True},
        {"name": "waveform_path", "label": "Waveform path (use # for channel)", "type": "string",
         "optional": True},
    ],
}

# Fields whose value names another file in the workspace (for the file browser
# to flag which files are referenced by configs).
FILE_REF_FIELDS = {"val_file", "string_file", "ext_shaped_sig_path", "h_pauli_mat",
                   "RF_freq_mat"}
# Fields that are *always* a plain filename in config_files (never a built-in
# Pauli symbol or an absolute/relative path). Only these drive "missing" flags,
# since h_pauli_mat/RF_freq_mat may legitimately be symbols like "X", "IZ", "J".
STRICT_FILE_FIELDS = {"val_file", "string_file"}


# --------------------------------------------------------------------------
# noise_config.json (for the shared noise generator)
# --------------------------------------------------------------------------
NOISE_MODES = ["colored", "arb"]
NOISE_FIELDS: List[Dict[str, Any]] = [
    {"name": "tag", "label": "Tag / group name", "type": "string"},
    {"name": "mode", "label": "Mode", "type": "enum", "options": NOISE_MODES},
    {"name": "channels", "label": "Channels", "type": "int"},
    {"name": "start_idx", "label": "Start index", "type": "int", "optional": True},
    {"name": "time_step", "label": "Time step (s)", "type": "float"},
    {"name": "length", "label": "Length (samples)", "type": "float"},
    {"name": "amplitude", "label": "Amplitude", "type": "float"},
]
# Extra field(s) required per mode.
NOISE_MODE_EXTRA: Dict[str, List[Dict[str, Any]]] = {
    "colored": [{"name": "alpha", "label": "Alpha (1/f exponent)", "type": "float"}],
    "arb": [{"name": "noise_expr", "label": "Noise spectrum expression f(f)",
             "type": "string", "help": "e.g. abs(f)^(-0.9)+0.0001*log(f)"}],
}


def get_schema() -> Dict[str, Any]:
    """JSON-serializable schema for the frontend."""
    return {
        "sim_fields": SIM_FIELDS,
        "sweep_fields": SWEEP_FIELDS,
        "sweep_classes": SWEEP_CLASSES,
        "gate_fields": GATE_FIELDS,
        "hamiltonian_types": HAMILTONIAN_TYPES,
        "noise_fields": NOISE_FIELDS,
        "noise_mode_extra": NOISE_MODE_EXTRA,
    }


# --------------------------------------------------------------------------
# Validation / coercion
# --------------------------------------------------------------------------
class ValidationError(ValueError):
    pass


def _coerce(value: Any, field: Dict[str, Any], where: str) -> Any:
    t = field["type"]
    name = field["name"]
    if t in ("string", "file", "enum"):
        if not isinstance(value, str):
            raise ValidationError(f"{where}.{name} must be a string")
        if t == "enum" and value not in field["options"]:
            raise ValidationError(
                f"{where}.{name} must be one of {field['options']}, got '{value}'")
        return value
    if t == "int":
        try:
            return int(value)
        except (TypeError, ValueError):
            raise ValidationError(f"{where}.{name} must be an integer")
    if t == "float":
        try:
            return float(value)
        except (TypeError, ValueError):
            raise ValidationError(f"{where}.{name} must be a number")
    if t == "bool":
        if isinstance(value, bool):
            return value
        raise ValidationError(f"{where}.{name} must be true/false")
    if t == "string_list":
        if not isinstance(value, list) or not all(isinstance(x, str) for x in value):
            raise ValidationError(f"{where}.{name} must be a list of strings")
        return value
    if t == "hamiltonian_multiselect":
        if not isinstance(value, list) or not all(isinstance(x, str) for x in value):
            raise ValidationError(f"{where}.{name} must be a list of Hamiltonian tags")
        return value
    return value


def _apply_fields(obj: Dict[str, Any], fields: List[Dict[str, Any]], where: str
                  ) -> Dict[str, Any]:
    out: Dict[str, Any] = {}
    for f in fields:
        name = f["name"]
        if name in obj and obj[name] is not None and not (
                f["type"] in ("string", "file") and obj[name] == "" and not f.get("optional")):
            out[name] = _coerce(obj[name], f, where)
        elif not f.get("optional"):
            # allow empty string for optional-ish text fields; otherwise require
            if f["type"] in ("string", "file"):
                out[name] = obj.get(name, "")
            else:
                raise ValidationError(f"{where}.{name} is required")
        elif name in obj:
            out[name] = _coerce(obj[name], f, where) if obj[name] != "" else obj[name]
    return out


def validate_sim_config(data: Dict[str, Any]) -> Dict[str, Any]:
    if not isinstance(data, dict):
        raise ValidationError("sim_config must be a JSON object")
    out = _apply_fields(data, SIM_FIELDS, "sim_config")
    rows = data.get("sweep_param_info", [])
    if not isinstance(rows, list):
        raise ValidationError("sweep_param_info must be a list")
    clean_rows = []
    for i, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValidationError(f"sweep_param_info[{i}] must be an object")
        where = f"sweep_param_info[{i}]"
        cls = row.get("class")
        if cls not in SWEEP_CLASSES:
            raise ValidationError(f"{where}.class must be one of {SWEEP_CLASSES}")
        r = {"class": cls, "tag": str(row.get("tag", "")),
             "property": str(row.get("property", ""))}
        val_file = str(row.get("val_file") or "")
        string_file = str(row.get("string_file") or "")
        # File type controls how the simulator decodes the vector: val_file ->
        # numeric, string_file -> string. Sequence is always string; Gate is
        # always numeric; Hamiltonian may sweep either a numeric or string property.
        if cls == "Sequence":
            if not string_file:
                raise ValidationError(f"{where}: Sequence sweep needs a string file")
            r["string_file"] = string_file
        elif cls == "Gate":
            if not r["property"]:
                raise ValidationError(f"{where}: Gate sweep needs a property")
            if not val_file:
                raise ValidationError(f"{where}: Gate sweep needs a numerical value file")
            r["val_file"] = val_file
        else:  # Hamiltonian
            if not r["property"]:
                raise ValidationError(f"{where}: Hamiltonian sweep needs a property")
            if val_file and string_file:
                raise ValidationError(
                    f"{where}: choose a numerical OR a string file, not both")
            if val_file:
                r["val_file"] = val_file
            elif string_file:
                r["string_file"] = string_file
            else:
                raise ValidationError(
                    f"{where}: Hamiltonian sweep needs a numerical or string file")
        clean_rows.append(r)
    out["sweep_param_info"] = clean_rows
    return out


def validate_gate_config(data: Dict[str, Any]) -> Dict[str, Any]:
    defs = data.get("gate_defs")
    if not isinstance(defs, list):
        raise ValidationError("gate_config must have a 'gate_defs' list")
    out_defs = []
    for i, g in enumerate(defs):
        if not isinstance(g, dict):
            raise ValidationError(f"gate_defs[{i}] must be an object")
        gg = dict(g)
        gg.setdefault("type", "switch")
        out_defs.append(_apply_fields(gg, GATE_FIELDS, f"gate_defs[{i}]"))
    return {"gate_defs": out_defs}


def validate_hamiltonian_config(data: Dict[str, Any]) -> Dict[str, Any]:
    defs = data.get("hamiltonian_prototype_defs")
    if not isinstance(defs, list):
        raise ValidationError(
            "hamiltonian_config must have a 'hamiltonian_prototype_defs' list")
    out_defs = []
    for i, h in enumerate(defs):
        if not isinstance(h, dict):
            raise ValidationError(f"hamiltonian_prototype_defs[{i}] must be an object")
        htype = h.get("type")
        if htype not in HAMILTONIAN_TYPES:
            raise ValidationError(
                f"hamiltonian_prototype_defs[{i}].type must be one of "
                f"{list(HAMILTONIAN_TYPES)}, got '{htype}'")
        where = f"hamiltonian_prototype_defs[{i}]"
        row = _apply_fields(h, HAMILTONIAN_TYPES[htype], where)
        row["type"] = htype
        out_defs.append(row)
    return {"hamiltonian_prototype_defs": out_defs}


def validate_noise_config(data: Dict[str, Any]) -> Dict[str, Any]:
    if not isinstance(data, dict):
        raise ValidationError("noise_config must be a JSON object")
    out = _apply_fields(data, NOISE_FIELDS, "noise_config")
    mode = out.get("mode")
    for f in NOISE_MODE_EXTRA.get(mode, []):
        if data.get(f["name"]) in (None, ""):
            raise ValidationError(
                f"noise_config.{f['name']} is required for mode '{mode}'")
        out[f["name"]] = _coerce(data[f["name"]], f, "noise_config")
    out.setdefault("start_idx", 0)
    if out["channels"] <= 0:
        raise ValidationError("noise_config.channels must be > 0")
    if out["length"] <= 0:
        raise ValidationError("noise_config.length must be > 0")
    if out["time_step"] <= 0:
        raise ValidationError("noise_config.time_step must be > 0")
    return out


VALIDATORS = {
    "sim": validate_sim_config,
    "gate": validate_gate_config,
    "hamiltonian": validate_hamiltonian_config,
}
CONFIG_FILENAMES = {
    "sim": "sim_config.json",
    "gate": "gate_config.json",
    "hamiltonian": "hamiltonian_config.json",
}
