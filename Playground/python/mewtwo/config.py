"""JSON configuration file loading.

Port of LoadJsonConfig.m — reads MewtwoMegaEvo's JSON configuration files
(sim_config.json, gate_config.json, hamiltonian_config.json).
"""

import json
import os
from typing import Any


def load_json_config(file_path: str) -> dict[str, Any]:
    """Load a JSON configuration file.

    Parameters
    ----------
    file_path : str
        Absolute path to the JSON file.

    Returns
    -------
    dict
        Parsed JSON content.
    """
    with open(file_path, "r") as f:
        return json.load(f)


def load_sim_config(data_path: str) -> dict[str, Any]:
    """Load sim_config.json from a result directory.

    Parameters
    ----------
    data_path : str
        Path to the simulation result directory (contains config_files/).

    Returns
    -------
    dict
        Parsed simulation configuration.
    """
    config_path = os.path.join(data_path, "config_files", "sim_config.json")
    return load_json_config(config_path)


def load_gate_config(data_path: str) -> dict[str, Any]:
    """Load gate_config.json from a result directory."""
    config_path = os.path.join(data_path, "config_files", "gate_config.json")
    return load_json_config(config_path)


def load_hamiltonian_config(data_path: str) -> dict[str, Any]:
    """Load hamiltonian_config.json from a result directory."""
    config_path = os.path.join(data_path, "config_files", "hamiltonian_config.json")
    return load_json_config(config_path)


def load_all_configs(
    data_path: str,
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    """Load all three config files from a result directory.

    Returns
    -------
    tuple[dict, dict, dict]
        (sim_config, gate_config, hamiltonian_config)
    """
    return (
        load_sim_config(data_path),
        load_gate_config(data_path),
        load_hamiltonian_config(data_path),
    )
