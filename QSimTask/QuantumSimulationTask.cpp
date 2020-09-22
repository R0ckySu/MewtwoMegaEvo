//
// Created by Rocky Su on 18/9/20.
//

#include "QuantumSimulationTask.h"

QSimTask::QSimTask() {

}

QSimTask::~QSimTask() {

}

void QSimTask::load_sim_configs() {
    std::string sim_configfile_path = std::string(config_file_folder).append("/sim_config.json");
    sim_configs = load_config_from_path(sim_configfile_path);
}

void QSimTask::load_gate_configs() {
    std::string gate_configfile_path = std::string(config_file_folder).append("/gate_config.json");
    gate_configs = load_config_from_path(gate_configfile_path);
    gate_prototype_map = std::map<gate_tag_type,Gate *>();

    std::vector<nlohmann::json> gate_defs = gate_configs["gate_defs"];
    for (auto & gate_def : gate_defs) {
        Gate *gate_new = new Gate(gate_def);
        gate_prototype_map.insert(std::make_pair(gate_new->tag, gate_new));
    }
}

void QSimTask::load_hamiltonian_configs() {
    std::string hamiltonian_configfile_path = std::string(config_file_folder).append("/hamiltonian_config.json");
    hamiltonian_configs = load_config_from_path(hamiltonian_configfile_path);
    hamiltonian_prototype_map = std::map<hamiltonian_tag_type,Hamiltonian *>();

    std::vector<nlohmann::json> h_prototypes_defs = hamiltonian_configs["hamiltonian_prototype_defs"];
    for (auto & h_prototypes_def : h_prototypes_defs) {
        std::string tag = h_prototypes_def["tag"];
        std::string hamiltonian_type = h_prototypes_def["type"];
        if (hamiltonian_type == "static") {
            auto *h_staic = new Static_Hamiltonian(h_prototypes_def);
            hamiltonian_prototype_map.insert(std::make_pair(tag, h_staic));
        } else if(hamiltonian_type == "mw") {
            auto *mw = new MW_Hamiltonian(h_prototypes_def);
            hamiltonian_prototype_map.insert(std::make_pair(tag,mw));
        } else if(hamiltonian_type == "awg") {

        } else if(hamiltonian_type == "noise") {
            auto *noise = new Noise_Hamiltonian(h_prototypes_def);
            hamiltonian_prototype_map.insert(std::make_pair(tag,noise));
        }
    }
}

nlohmann::json QSimTask::load_config_from_path(const std::string& path) {
    nlohmann::json config_json;
    std::ifstream file(path);
    if (file) {
        file >> config_json;
        file.close();
    }
    return config_json;
}

void QSimTask::load_sequence() {

}
