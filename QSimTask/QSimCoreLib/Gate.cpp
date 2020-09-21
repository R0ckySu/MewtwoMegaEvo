//
// Created by Rocky Su on 17/9/20.
//

#include "Gate.h"

Gate::Gate() {
    tag = "";
    hamiltonian_tags_list = std::vector<std::string>();
}

Gate::Gate(const TimingBasic &t, const Gate &g):TimingBasic(t) {
    tag = g.tag;
    hamiltonian_tags_list = g.hamiltonian_tags_list;
}


