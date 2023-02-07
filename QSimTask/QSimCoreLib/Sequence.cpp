//
// Created by Rocky Su on 18/9/20.
//

#include "Sequence.h"
#include <regex>
#include "Utils.h"

/****************************************************/
Sequence::Sequence() {
    sequential_gate_list = std::vector<Gate *>();
    measurement_time_point_vec = arma::vec();
    active_gate_tag_list = std::vector<std::string>();
    gate_switching_map = std::map<gate_tag_type, arma::vec>();
}

Sequence::~Sequence() {
    std::map<gate_tag_type, arma::vec>().swap(gate_switching_map);
    std::vector<Gate *>().swap(sequential_gate_list);
    for (int i = 0; i < sequential_gate_list.size(); ++i) {
        delete sequential_gate_list[i];
    }
    arma::vec().swap(measurement_time_point_vec);
    std::vector<std::string>().swap(active_gate_tag_list);
}

void Sequence::generate_switching_sig() {

    gate_switching_map = std::map<gate_tag_type, arma::vec>();
    std::vector<double> meas_time_point_temp = std::vector<double>();

    std::cout << "Sequence: switching sig length: " << get_total_num_steps() << std::endl;

    for (int i = 0; i < sequential_gate_list.size(); ++i) {
        Gate *gate_unit = sequential_gate_list.at(i);
        // Record virtual measurement gate time.
        if (gate_unit->tag == std::string("M")) {
            meas_time_point_temp.push_back(gate_unit->start_time);
        }
        //Skip FID gates and virtual gates
        if (gate_unit->hamiltonian_tags_list.empty()) {
            continue;
        }

        std::cout << "Sequence:" << gate_unit->tag << " turn on from " << gate_unit->get_start_index() << "~" << gate_unit->get_end_index() << std::endl;
        active_gate_tag_list.push_back(std::string(gate_unit->tag));
        if (gate_switching_map.find(gate_unit->tag) == gate_switching_map.end()) {
            std::cout << "Sequence:" << gate_unit->tag << " generating new switching" << std::endl;
            arma::vec new_switching = arma::vec(get_total_num_steps()).fill(0);
            std::pair<gate_tag_type, arma::vec> new_gate_swicthing_entry = std::pair<gate_tag_type, arma::vec>(gate_unit->tag,new_switching);
            gate_switching_map.insert(new_gate_swicthing_entry);
        }

        if (empty(gate_unit->ext_shaped_sig_path)) {
            gate_switching_map[gate_unit->tag].subvec(gate_unit->get_start_index(),gate_unit->get_end_index()).fill(1);
        } else {
            gate_switching_map[gate_unit->tag].subvec(gate_unit->get_start_index(),gate_unit->get_end_index()) = gate_unit->ext_shaped_sig;
        }
    }

//    std::cout << "Gate Jgate: \n"<< gate_switching_map["Jgate"] << std::endl;
    measurement_time_point_vec = arma::vec(meas_time_point_temp);
}

void Sequence::append_gate(const Gate &g) {
    Gate *newGate = new Gate(g);
    if(!sequential_gate_list.empty()) {
        Gate *pre_neighbor = sequential_gate_list.at(sequential_gate_list.size() - 1);
        newGate->shift_by_time(pre_neighbor->end_time + newGate->seq_shift_time);
        end_time = std::max(newGate->end_time, pre_neighbor->end_time);
        sequential_gate_list.push_back(newGate);
    } else {
        sequential_gate_list.push_back(newGate);
        end_time = newGate->end_time;
    }
//    end_time += newGate->get_pulse_width();

    std::cout << newGate->description().name << newGate->description().data_row << std::endl;
}

void Sequence::append_sequence(const Sequence & seq) {
    for (int i = 0; i < seq.sequential_gate_list.size(); ++i) {
        append_gate(*seq.sequential_gate_list.at(i));
    }
}

void Sequence::load_sequence(double _step_size,
                        std::string sequence_string,
                        std::map<std::string, std::string> symbol_alias,
                        std::map<std::string, Gate *> gate_prototype_map) {
    step_size = _step_size;

    // Replace string alias from the symbol_alia map
    std::string preprocessed_seq_str = sequence_string;
    if (symbol_alias.begin() != symbol_alias.end()) {
        for (const auto& symbol_alias_pair : symbol_alias) {
            preprocessed_seq_str = find_and_replace_string(symbol_alias_pair.first,symbol_alias_pair.second,preprocessed_seq_str);
        }
    }
    std::cout << "Sequence: Symbol alias replaced! processed seq str:\n" << preprocessed_seq_str << std::endl;

    // Decode symbolic sequence
    auto gate_info_pair_vec = symbolic_sequence_str_parser(preprocessed_seq_str);
    for (const auto& info_pair : gate_info_pair_vec) {
        std::string gate_tag = info_pair.first;
        std::string gate_param_str = info_pair.second;
//        if (not gate_tag.compare("M") || *gate_prototype_map.find(gate_tag) != *gate_prototype_map.end() ) {
            // Gate found
        std::cout << "Sequence: Loading " << gate_tag << std::endl;
        Gate * gate_new = new Gate(*gate_prototype_map[gate_tag]);
        if (!gate_param_str.empty()) {gate_new->decode_param_str(gate_param_str);}
        append_gate(*gate_new);
//        }
    }

    generate_switching_sig();
}

int Sequence::get_total_num_steps() {
    return ceil(end_time/step_size);
}
