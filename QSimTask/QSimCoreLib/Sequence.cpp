//
// Created by Rocky Su on 18/9/20.
//

#include "Sequence.h"
#include <regex>
#include "Utils.h"

Sequence::Sequence() {
    sequential_gate_list = std::vector<Gate *>();
}

void Sequence::generate_switching_sig() {

    gate_switching_map = std::map<gate_tag_type, arma::vec>();

    for (int i = 0; i < sequential_gate_list.size(); ++i) {
        Gate *gate_unit = sequential_gate_list.at(i);
        //Skip FID gates
        if (gate_unit->hamiltonian_tags_list.empty()) {
            continue;
        }
        if (gate_switching_map.find(gate_unit->tag) != gate_switching_map.end()) {
            gate_switching_map[gate_unit->tag].subvec(floor(gate_unit->start_time/this->step_size),floor(gate_unit->end_time/this->step_size)).fill(1);
        } else {
            arma::vec new_switching = arma::vec(get_total_num_steps()).fill(0);
            new_switching.subvec(floor(gate_unit->start_time/this->step_size),floor(gate_unit->end_time/this->step_size)).fill(1);
            std::pair<gate_tag_type, arma::vec> new_gate_swicthing_entry = std::pair<gate_tag_type, arma::vec>(gate_unit->tag,new_switching);
            gate_switching_map.insert(new_gate_swicthing_entry);
        }
    }
}

void Sequence::append_gate(const Gate &g) {
    Gate *newGate = new Gate(g);
    if(!sequential_gate_list.empty()) {
        Gate *pre_neighbor = sequential_gate_list.at(sequential_gate_list.size() - 1);
        newGate->shift_by_time(pre_neighbor->end_time);
        sequential_gate_list.push_back(newGate);
    } else {
        sequential_gate_list.push_back(newGate);
    }
    end_time += newGate->pulse_width;

    std::cout << newGate->description().name << newGate->description().data_row << std::endl;
}

void Sequence::append_sequence(const Sequence & seq) {
    for (int i = 0; i < seq.sequential_gate_list.size(); ++i) {
        append_gate(*seq.sequential_gate_list.at(i));
    }
}

void Sequence::test() {
    set_pulse_width(200e-7);
    step_size = 1e-7;

    MW_Hamiltonian X1 = MW_Hamiltonian();
    X1.tag = "X1";
    X1.h_mat = arma::cx_mat(arma::ones(4,4),arma::zeros(4,4));
    X1.amplitude = 1.5e1;
    X1.phase = M_PI/2;
    X1.freq = 1e4;
    X1.num_of_steps = get_total_num_steps();
    X1.step_size = step_size;

    Gate gate1 = Gate();
    gate1.tag = "G1";
    gate1.set_pulse_width(1e-6);
    gate1.step_size = step_size;
    append_gate(gate1);
    append_gate(gate1);

    Gate gate2 = Gate();
    gate2.tag = "G2";
    gate2.set_pulse_width(3e-6);
    gate2.step_size = step_size;
    append_gate(gate2);

    generate_switching_sig();

    X1.switching_signal = gate_switching_map["G1"];
    X1.load_waveform();
    std::cout << X1.wave_form << std::endl;
//    std::cout << "G1 sw:\n" << gate_switching_map["G1"] << std::endl;
//    std::cout << "G2 sw:\n" << gate_switching_map["G2"] << std::endl;
}

