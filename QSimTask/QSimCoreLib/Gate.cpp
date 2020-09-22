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

Gate::Gate(nlohmann::json gate_config) {
    tag = gate_config["tag"];
    std::vector<std::string> h_tag_list = gate_config["hamiltonians"];
    hamiltonian_tags_list = h_tag_list;
    this->set_pulse_width(gate_config["gate_length"]);
}


