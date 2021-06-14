//
// Created by Rocky Su on 6/11/20.
//

#ifndef MEWTWOMEGAEVO_SIMPROTOTYPES_H
#define MEWTWOMEGAEVO_SIMPROTOTYPES_H

#include "nlohmann/json.hpp"
#include "Hamiltonian.h"
#include "Gate.h"

struct HamiltonianPrototypes {
    HamiltonianPrototypes();
    HamiltonianPrototypes(const HamiltonianPrototypes &h);
    HamiltonianPrototypes(nlohmann::json h_configs, std::string config_folder);

    std::map<hamiltonian_tag_type, MW_Hamiltonian *> mw_hamiltonian_prototype_map;
    std::map<hamiltonian_tag_type, AWG_Hamiltonian *> awg_hamiltonian_prototype_map;
    std::map<hamiltonian_tag_type, Static_Hamiltonian *> static_hamiltonian_prototype_map;
    std::map<hamiltonian_tag_type, Noise_Hamiltonian *> noise_hamiltonian_prototype_map;

    void update_prototype_with(std::string tag_name, std::string property_name, double value);
};

struct GatePrototypes {
    GatePrototypes();
    GatePrototypes(const GatePrototypes &g);
    GatePrototypes(nlohmann::json h_configs, std::string config_folder);

    double time_step;
    std::map<gate_tag_type, Gate *> gate_prototype_map;
    void update_prototype_with(std::string tag_name, std::string property_name, double value);
};


struct SimPrototypes {
    SimPrototypes();
    SimPrototypes(const SimPrototypes &s);

//    std::map<gate_tag_type, Gate *> gate_prototype_map;

    HamiltonianPrototypes hamiltonian_prototypes;
    GatePrototypes gate_prototypes;

    std::map<std::string, std::string> sequence_symbol_alias_map;

//    void update_prototype_with(std::string tag_name, std::string property_name, double value);
};


#endif
