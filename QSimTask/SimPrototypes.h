//
// Created by Rocky Su on 6/11/20.
//

#ifndef MEWTWOMEGAEVO_SIMPROTOTYPES_H
#define MEWTWOMEGAEVO_SIMPROTOTYPES_H

#include "QSimCoreLib/QSimCore.h"

struct SimPrototypes {
    SimPrototypes();
    ~SimPrototypes();
    SimPrototypes(const SimPrototypes &s);
    std::map<gate_tag_type, Gate *> gate_prototype_map;
    std::map<hamiltonian_tag_type, Hamiltonian *> ctrl_hamiltonian_prototype_map;
    std::map<hamiltonian_tag_type, Noise_Hamiltonian *> noise_hamiltonian_prototype_map;
    std::map<std::string, std::string> sequence_symbol_alias_map;
};


#endif //MEWTWOMEGAEVO_SIMPROTOTYPES_H
