//
// Created by Rocky Su on 6/11/20.
//

#include "SimPrototypes.h"

SimPrototypes::SimPrototypes() {
    ctrl_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Hamiltonian *>();
    noise_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Noise_Hamiltonian *>();
    gate_prototype_map = std::map<gate_tag_type, Gate *>();
    sequence_symbol_alias_map = std::map<std::string, std::string>();
};

SimPrototypes::~SimPrototypes() {
    std::cout << "Sim Prototype Cleaning" << std::endl;
    for (const auto& ctrl_h_item :ctrl_hamiltonian_prototype_map) {
        ctrl_h_item.second->clean_up_on_reload();
        delete ctrl_h_item.second;
    }

    for (const auto& noise_h_item :noise_hamiltonian_prototype_map) {
        noise_h_item.second->clean_up_on_reload();
        delete noise_h_item.second;
    }

    for (const auto& gate_item :gate_prototype_map) {
        delete gate_item.second;
    }
};

SimPrototypes::SimPrototypes(const SimPrototypes &s) {
    //Copy constructer
    ctrl_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Hamiltonian *>();
    for (const auto& ctrl_h_item : s.ctrl_hamiltonian_prototype_map) {
        ctrl_hamiltonian_prototype_map.insert(std::make_pair(ctrl_h_item.first,ctrl_h_item.second->clone()));
    }

    noise_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Noise_Hamiltonian *>();
    for (const auto& noise_h_item : s.noise_hamiltonian_prototype_map) {
        noise_hamiltonian_prototype_map.insert(std::make_pair(noise_h_item.first,noise_h_item.second->clone()));
    }

    gate_prototype_map = std::map<gate_tag_type, Gate *>();
    for (const auto& gate_item : s.gate_prototype_map) {
        gate_prototype_map.insert(std::make_pair(gate_item.first,gate_item.second->clone()));
    }

    sequence_symbol_alias_map = std::map<std::string, std::string>();
    for (const auto& symbol_item : s.sequence_symbol_alias_map) {
        sequence_symbol_alias_map.insert(std::make_pair(symbol_item.first,symbol_item.second));
    }
}