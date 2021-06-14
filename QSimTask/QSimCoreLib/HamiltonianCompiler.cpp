//
// Created by Rocky Su on 14/6/21.
//

#include "HamiltonianCompiler.h"


HamiltonianCompiler::HamiltonianCompiler() {

}

void HamiltonianCompiler::load_ctrl_signals_from_sequence(Sequence &seq, GatePrototypes g_proto) {
    total_num_steps = seq.get_total_num_steps();
    for (const auto& gate_item : g_proto.gate_prototype_map) {
        auto gate_proto_tag = gate_item.first;
        auto gate_proto_obj = gate_item.second;

        for (const auto& binded_hamiltonian_tag : gate_proto_obj->hamiltonian_tags_list) {
            Gated_Hamiltonian * hamiltonian_obj;
            if (h_prototype_ptr.awg_hamiltonian_prototype_map.find(binded_hamiltonian_tag) != h_prototype_ptr.awg_hamiltonian_prototype_map.end()){
                hamiltonian_obj = h_prototype_ptr.awg_hamiltonian_prototype_map[binded_hamiltonian_tag];
            }
            else if (h_prototype_ptr.mw_hamiltonian_prototype_map.find(binded_hamiltonian_tag) != h_prototype_ptr.mw_hamiltonian_prototype_map.end()){
                hamiltonian_obj = h_prototype_ptr.mw_hamiltonian_prototype_map[binded_hamiltonian_tag];
            }
            hamiltonian_obj->add_signal(seq.gate_switching_map[gate_proto_tag]);
        }
    }
    std::cout << "HamiltonianCompiler: ctrl signals are loaded" << std::endl;
}

arma::cx_cube *HamiltonianCompiler::compile_ctrl_hamiltonian() {

    auto * ctrl_hamiltonian_time_dep = new arma::cx_cube(system_dim,system_dim,total_num_steps);
    ctrl_hamiltonian_time_dep->fill(0);

    for (const auto& s_hamiltonian_item : h_prototype_ptr.static_hamiltonian_prototype_map) {
        s_hamiltonian_item.second->num_of_steps = total_num_steps;
        s_hamiltonian_item.second->step_size = step_size;
        s_hamiltonian_item.second->load_waveform();
        s_hamiltonian_item.second->fetch_H(ctrl_hamiltonian_time_dep);
    }

    for (const auto& awg_hamiltonian_item : h_prototype_ptr.awg_hamiltonian_prototype_map) {
        awg_hamiltonian_item.second->num_of_steps = total_num_steps;
        awg_hamiltonian_item.second->step_size = step_size;
        awg_hamiltonian_item.second->load_waveform();
        awg_hamiltonian_item.second->fetch_H(ctrl_hamiltonian_time_dep);
    }

    for (const auto& mw_hamiltonian_item : h_prototype_ptr.mw_hamiltonian_prototype_map) {
        mw_hamiltonian_item.second->num_of_steps = total_num_steps;
        mw_hamiltonian_item.second->step_size = step_size;
        mw_hamiltonian_item.second->load_waveform();
        mw_hamiltonian_item.second->fetch_H(ctrl_hamiltonian_time_dep);
    }

    std::cout << "HamiltonianCompiler: ctrl Hamiltonians are compiled!" << std::endl;

    return ctrl_hamiltonian_time_dep;
}

arma::cx_cube *
HamiltonianCompiler::compile_noise_hamiltonian(int noise_idx, std::vector<double> random_start_pos_factor) {
    arma::cx_cube *noise_hamiltonian_time_dep = new arma::cx_cube(system_dim,system_dim,total_num_steps);
    noise_hamiltonian_time_dep->fill(0);
    for (const auto& noise_hamiltonian_item : h_prototype_ptr.noise_hamiltonian_prototype_map) {
        auto * noise_h_temp = new Noise_Hamiltonian(*noise_hamiltonian_item.second);
        noise_h_temp->num_of_steps = total_num_steps;
        noise_h_temp->step_size = step_size;
        noise_h_temp->randomStartPosFactor = random_start_pos_factor.at(noise_idx);
        noise_h_temp->load_ext_waveform(noise_idx);
        noise_h_temp->fetch_H(noise_hamiltonian_time_dep);
    }
    std::cout << "HamiltonianCompiler: noise Hamiltonian compiled!" << std::endl;

    return noise_hamiltonian_time_dep;
}