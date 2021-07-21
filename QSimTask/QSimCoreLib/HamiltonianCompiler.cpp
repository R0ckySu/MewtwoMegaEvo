//
// Created by Rocky Su on 14/6/21.
//

#include "HamiltonianCompiler.h"
#include "Utils.h"

EigenSysTimeDepStruct::EigenSysTimeDepStruct(){};

EigenSysTimeDepStruct::EigenSysTimeDepStruct(uint num_of_steps, uint sys_size){
    frame_trans_pos = arma::uvec(num_of_steps);
    eigen_energy_time_dep = arma::mat(sys_size, num_of_steps).fill(0.0);;
    basis_trans_time_dep = arma::cx_cube(sys_size,sys_size, num_of_steps).fill(0.0);

    residual_phase_H_time_dep = arma::cx_cube(sys_size,sys_size, num_of_steps).fill(0.0);
    residual_phase_H_time_dep.slice(0) = arma::expmat(arma::cx_mat(arma::zeros(sys_size,sys_size),arma::zeros(sys_size,sys_size)));

    eigen_freq_mask_time_dep = arma::cube(sys_size,sys_size, num_of_steps).fill(0.0);
}

HamiltonianCompiler::HamiltonianCompiler() {

}


HamiltonianCompiler::~HamiltonianCompiler() {
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
    std::cout << "HamiltonianCompiler: ctrl hamiltonian compiling" << std::endl;
    auto * ctrl_hamiltonian_time_dep = new arma::cx_cube(system_dim,system_dim,total_num_steps);
    ctrl_hamiltonian_time_dep->fill(0);
    if (turn_on_dynamic_frame_trans == false) {
        for (const auto& s_hamiltonian_item : h_prototype_ptr.static_hamiltonian_prototype_map) {
            std::cout << "HamiltonianCompiler: compiling static:" << s_hamiltonian_item.second->tag << std::endl;
            s_hamiltonian_item.second->num_of_steps = total_num_steps;
            s_hamiltonian_item.second->step_size = step_size;
            s_hamiltonian_item.second->load_waveform();
            s_hamiltonian_item.second->fetch_H(ctrl_hamiltonian_time_dep);
        }

        for (const auto& awg_hamiltonian_item : h_prototype_ptr.awg_hamiltonian_prototype_map) {
            std::cout << "HamiltonianCompiler: compiling awg:" << awg_hamiltonian_item.second->tag << std::endl;
            awg_hamiltonian_item.second->num_of_steps = total_num_steps;
            awg_hamiltonian_item.second->step_size = step_size;
            awg_hamiltonian_item.second->load_waveform();
            awg_hamiltonian_item.second->fetch_H(ctrl_hamiltonian_time_dep);
        }

        for (const auto& mw_hamiltonian_item : h_prototype_ptr.mw_hamiltonian_prototype_map) {
            std::cout << "HamiltonianCompiler: compiling mw:" << mw_hamiltonian_item.second->tag << std::endl;
            mw_hamiltonian_item.second->num_of_steps = total_num_steps;
            mw_hamiltonian_item.second->step_size = step_size;
            mw_hamiltonian_item.second->load_waveform();
            mw_hamiltonian_item.second->fetch_H(ctrl_hamiltonian_time_dep);
        }
    } else {
        generate_dynamic_frame2();
        for (const auto& mw_hamiltonian_item : h_prototype_ptr.mw_hamiltonian_prototype_map) {
            mw_hamiltonian_item.second->num_of_steps = total_num_steps;
            mw_hamiltonian_item.second->step_size = step_size;
            mw_hamiltonian_item.second->load_and_fetch_H_with_dynamic_frame(ctrl_hamiltonian_time_dep);
        }
        std::cout << "HamiltonianCompiler: ctrl Hamiltonians under time-dep frame are compiled!" << std::endl;
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

//void HamiltonianCompiler::generate_dynamic_frame() {
//
//    arma::vec total_switching_sig = arma::vec(total_num_steps).fill(0);
//    for (const auto& awg_hamiltonian_item : h_prototype_ptr.awg_hamiltonian_prototype_map) {
//        total_switching_sig = total_switching_sig+awg_hamiltonian_item.second->switching_signal;
//    }
////    std::cout << "Hamiltonian Compiler: tot sw function: \n" <<total_switching_sig << std::endl;
//    arma::uvec transform_positions = arma::find(arma::diff(total_switching_sig));
//    transform_positions = transform_positions + 1;
//    transform_positions.insert_rows(0,1);
//    transform_positions.at(0) = 0;
//
//    eigen_sys = EigenSysTimeDepStruct(transform_positions.n_elem, system_dim);
//    eigen_sys.time_step = step_size;
//    eigen_sys.frame_trans_pos = transform_positions;
//    std::cout << "Hamiltonian Compiler: Positions for frame trans:\n" << eigen_sys.frame_trans_pos << std::endl;
//
//    auto * static_hamiltonian_time_dep = new arma::cx_cube(system_dim,system_dim, transform_positions.n_elem);
//    static_hamiltonian_time_dep->fill(0);
//    for (const auto& s_hamiltonian_item : h_prototype_ptr.static_hamiltonian_prototype_map) {
//        for (int i = 0; i < transform_positions.n_elem; ++i) {
//            static_hamiltonian_time_dep->slice(i) += s_hamiltonian_item.second->amplitude * s_hamiltonian_item.second->h_mat.mat;
//        }
//    }
//
//    auto * awg_hamiltonian_time_dep = new arma::cx_cube(system_dim,system_dim, transform_positions.n_elem);
//    awg_hamiltonian_time_dep->fill(0);
//    for (const auto& awg_hamiltonian_item : h_prototype_ptr.awg_hamiltonian_prototype_map) {
//        for (int i = 0; i < transform_positions.n_elem; ++i) {
//            awg_hamiltonian_time_dep->slice(i) += awg_hamiltonian_item.second->switching_signal(transform_positions(i)) * awg_hamiltonian_item.second->amplitude * awg_hamiltonian_item.second->h_mat.mat;
//        }
//    }
//
//    // Force setting the second point to be diagonalised
//
//    for (uint i = 0; i < transform_positions.n_elem; i++) {
//        arma::dvec E_eig_temp;
//        arma::cx_mat U_eig_temp;
//
//        std::cout << "Frame Hamiltonian:\n" << "pos:" << transform_positions(i) << "\n" << static_hamiltonian_time_dep->slice(i)+awg_hamiltonian_time_dep->slice(i) << std::endl;
//
//        arma::eig_sym(E_eig_temp,U_eig_temp, static_hamiltonian_time_dep->slice(i)+awg_hamiltonian_time_dep->slice(i) , "std");
//        //Reverse to descending order
//        E_eig_temp = arma::reverse(E_eig_temp);
//        U_eig_temp = arma::reverse(U_eig_temp);
//        eigen_sys.eigen_energy_time_dep.col(i) = E_eig_temp;
//        eigen_sys.basis_trans_time_dep.slice(i) = U_eig_temp;
//        eigen_sys.eigen_freq_mask_time_dep.slice(i) = arma::kron(arma::ones(1,system_dim), E_eig_temp.as_col())-arma::kron(E_eig_temp.as_row(), arma::ones(system_dim,1));
//
//        std::cout << "Eigen Energy :\n" << E_eig_temp << std::endl;
//        std::cout << "Basis Trans Mat:\n" << U_eig_temp << std::endl;
//
//        if (i >= 1) {
//
//            int t_index = transform_positions(i);
//            // Residual phase
//            eigen_sys.residual_phase_time_dep.slice(i) = cal_residual_phase(t_index*step_size ,step_size,
//                                                                            eigen_sys.eigen_energy_time_dep.col(i),eigen_sys.eigen_energy_time_dep.col(i-1),
//                                                                            eigen_sys.basis_trans_time_dep.slice(i),eigen_sys.basis_trans_time_dep.slice(i-1));
//
//            std::cout << "Tot Residual Phase:\n" << eigen_sys.residual_phase_time_dep.slice(i) << std::endl;
//        }
//    }
//
//    // Transform the interaction Hamiltonians into new time-dependent basis
//    for (const auto& m_hamiltonian_item : h_prototype_ptr.mw_hamiltonian_prototype_map) {
//        m_hamiltonian_item.second->h_mat_under_time_dep_frame = arma::cx_cube(system_dim, system_dim, transform_positions.n_elem);
//        m_hamiltonian_item.second->freq_mask_time_dep = eigen_sys.eigen_freq_mask_time_dep;
//        m_hamiltonian_item.second->frame_trans_time_pos = eigen_sys.frame_trans_pos;
//        for (int i = 0; i < transform_positions.n_elem; ++i) {
//            m_hamiltonian_item.second->h_mat_under_time_dep_frame.slice(i) = arma::inv(eigen_sys.basis_trans_time_dep.slice(i)) *  m_hamiltonian_item.second->h_mat.mat * eigen_sys.basis_trans_time_dep.slice(i);
//        }
//    }
//
//    for (const auto& n_hamiltonian_item : h_prototype_ptr.noise_hamiltonian_prototype_map) {
//        n_hamiltonian_item.second->h_mat_under_time_dep_frame = arma::cx_cube(system_dim, system_dim, transform_positions.n_elem);
//        n_hamiltonian_item.second->frame_trans_time_pos = eigen_sys.frame_trans_pos;
//        for (int i = 0; i < transform_positions.n_elem; ++i) {
//            n_hamiltonian_item.second->h_mat_under_time_dep_frame.slice(i) = arma::inv(eigen_sys.basis_trans_time_dep.slice(i)) *  n_hamiltonian_item.second->h_mat.mat * eigen_sys.basis_trans_time_dep.slice(i);
//        }
//    }
//    std::cout << "HamiltonianCompiler: Time-dep frame generated!" << std::endl;
//}

void HamiltonianCompiler::generate_dynamic_frame2() {

    arma::vec total_switching_sig = arma::vec(total_num_steps).fill(0);
    for (const auto& awg_hamiltonian_item : h_prototype_ptr.awg_hamiltonian_prototype_map) {
        total_switching_sig = total_switching_sig+awg_hamiltonian_item.second->switching_signal;
    }
//    std::cout << "Hamiltonian Compiler: tot sw function: \n" <<total_switching_sig << std::endl;
    arma::uvec transform_positions = arma::find(arma::diff(total_switching_sig));
    transform_positions = transform_positions + 1;
    transform_positions.insert_rows(0,1);
    transform_positions.at(0) = 0;

    eigen_sys = EigenSysTimeDepStruct(transform_positions.n_elem, system_dim);
    eigen_sys.time_step = step_size;
    eigen_sys.frame_trans_pos = transform_positions;
    std::cout << "Hamiltonian Compiler: Positions for frame trans:\n" << eigen_sys.frame_trans_pos << std::endl;

    auto * static_hamiltonian_time_dep = new arma::cx_cube(system_dim,system_dim, transform_positions.n_elem);
    static_hamiltonian_time_dep->fill(0);
    for (const auto& s_hamiltonian_item : h_prototype_ptr.static_hamiltonian_prototype_map) {
        for (int i = 0; i < transform_positions.n_elem; ++i) {
            static_hamiltonian_time_dep->slice(i) += s_hamiltonian_item.second->amplitude * s_hamiltonian_item.second->h_mat.mat;
        }
    }

    auto * awg_hamiltonian_time_dep = new arma::cx_cube(system_dim,system_dim, transform_positions.n_elem);
    awg_hamiltonian_time_dep->fill(0);
    for (const auto& awg_hamiltonian_item : h_prototype_ptr.awg_hamiltonian_prototype_map) {
        for (int i = 0; i < transform_positions.n_elem; ++i) {
            awg_hamiltonian_time_dep->slice(i) += awg_hamiltonian_item.second->switching_signal(transform_positions(i)) * awg_hamiltonian_item.second->amplitude * awg_hamiltonian_item.second->h_mat.mat;
        }
    }

    residual_phase_propagator_time_dep = arma::cx_cube(system_dim,system_dim, transform_positions.n_elem).fill(0.0);
    // Force setting the second point to be diagonalised

    //Loop into transition points;
    for (uint i = 0; i < transform_positions.n_elem; i++) {
        residual_phase_propagator_time_dep.slice(i) = arma::cx_mat(arma::eye(system_dim,system_dim),arma::zeros(system_dim,system_dim));
        //Loop into finer steps in each transition
        uint num_of_interleaving = 1000;
        EigenSysTimeDepStruct eigen_sys_temp = EigenSysTimeDepStruct(num_of_interleaving, system_dim);
        arma::dvec E_eig_temp;
        arma::cx_mat U_eig_temp;
        for (uint j = 0; j < num_of_interleaving; j++) {
            double increment_factor = (float)j / (float)num_of_interleaving;
            double fine_step_size = step_size /num_of_interleaving;
            double time_abs = eigen_sys.frame_trans_pos(i) * step_size + j*fine_step_size;

//            std::cout << "Frame Hamiltonian:\n" << "pos:" << transform_positions(i)+increment_factor << "\n" << static_hamiltonian_time_dep->slice(i)+increment_factor*awg_hamiltonian_time_dep->slice(i) << std::endl;

            arma::eig_sym(E_eig_temp,U_eig_temp, static_hamiltonian_time_dep->slice(i)+increment_factor*awg_hamiltonian_time_dep->slice(i) , "std");
            //Reverse to descending order
            E_eig_temp = arma::reverse(E_eig_temp);
            U_eig_temp = arma::reverse(U_eig_temp);

            if (j==0 && i==0) {
                eigen_sys_temp.eigen_energy_time_dep.col(j) = E_eig_temp;
                eigen_sys_temp.basis_trans_time_dep.slice(j) = U_eig_temp;
            } else if (j==0 && i>0) {
                eigen_sys_temp.eigen_energy_time_dep.col(j) = eigen_sys.eigen_energy_time_dep.col(i-1);
                eigen_sys_temp.basis_trans_time_dep.slice(j) = eigen_sys.basis_trans_time_dep.slice(i-1);
            } else {
                eigen_sys_temp.eigen_energy_time_dep.col(j) = E_eig_temp;
                eigen_sys_temp.basis_trans_time_dep.slice(j) = U_eig_temp;
            }


            if (j >= 1) {
                // Fine step-in Residual phase
                arma::cx_mat res_phase = cal_residual_phase(time_abs ,fine_step_size,
                                                             eigen_sys_temp.eigen_energy_time_dep.col(j),eigen_sys_temp.eigen_energy_time_dep.col(j-1),
                                                             eigen_sys_temp.basis_trans_time_dep.slice(j),eigen_sys_temp.basis_trans_time_dep.slice(j-1));
                residual_phase_propagator_time_dep.slice(i) = residual_phase_propagator_time_dep.slice(i) * qmt::eig_matrix_exp(-arma::cx_double(0,1)*res_phase);
            }
        }

        eigen_sys.eigen_energy_time_dep.col(i) = eigen_sys_temp.eigen_energy_time_dep.col(num_of_interleaving-1);
        eigen_sys.basis_trans_time_dep.slice(i) = eigen_sys_temp.basis_trans_time_dep.slice(num_of_interleaving-1);
        eigen_sys.eigen_freq_mask_time_dep.slice(i) =  arma::kron(arma::ones(1,system_dim), eigen_sys.eigen_energy_time_dep.col(i).as_col())-arma::kron(eigen_sys.eigen_energy_time_dep.col(i).as_row(), arma::ones(system_dim,1));
        std::cout << "Eigen Energy :\n" << eigen_sys.eigen_energy_time_dep.col(i) << std::endl;
        std::cout << "Basis Trans Mat:\n" << eigen_sys.basis_trans_time_dep.slice(i) << std::endl;
        std::cout << "HamiltonianCompiler:: Residual phase propagator:\n" << residual_phase_propagator_time_dep.slice(i) << std::endl;
    }

    // Transform the interaction Hamiltonians into new time-dependent basis
    for (const auto& m_hamiltonian_item : h_prototype_ptr.mw_hamiltonian_prototype_map) {
        m_hamiltonian_item.second->h_mat_under_time_dep_frame = arma::cx_cube(system_dim, system_dim, transform_positions.n_elem);
        m_hamiltonian_item.second->freq_mask_time_dep = eigen_sys.eigen_freq_mask_time_dep;
        m_hamiltonian_item.second->frame_trans_time_pos = eigen_sys.frame_trans_pos;
        for (int i = 0; i < transform_positions.n_elem; ++i) {
            m_hamiltonian_item.second->h_mat_under_time_dep_frame.slice(i) = arma::inv(eigen_sys.basis_trans_time_dep.slice(i)) *  m_hamiltonian_item.second->h_mat.mat * eigen_sys.basis_trans_time_dep.slice(i);
        }
    }

    for (const auto& n_hamiltonian_item : h_prototype_ptr.noise_hamiltonian_prototype_map) {
        n_hamiltonian_item.second->h_mat_under_time_dep_frame = arma::cx_cube(system_dim, system_dim, transform_positions.n_elem);
        n_hamiltonian_item.second->frame_trans_time_pos = eigen_sys.frame_trans_pos;
        for (int i = 0; i < transform_positions.n_elem; ++i) {
            n_hamiltonian_item.second->h_mat_under_time_dep_frame.slice(i) = arma::inv(eigen_sys.basis_trans_time_dep.slice(i)) *  n_hamiltonian_item.second->h_mat.mat * eigen_sys.basis_trans_time_dep.slice(i);
        }
    }
    std::cout << "HamiltonianCompiler: Time-dep frame generated!" << std::endl;
}


std::pair<arma::uvec *, arma::cx_cube *> HamiltonianCompiler::get_basis_trans() {
    auto *pos_temp = new arma::uvec(eigen_sys.frame_trans_pos);
    auto *basis_temp = new arma::cx_cube(eigen_sys.basis_trans_time_dep);
    return std::pair<arma::uvec *, arma::cx_cube *>(pos_temp, basis_temp);
}

std::pair<arma::uvec *, arma::cx_cube *> HamiltonianCompiler::get_residual_transition_propagator() {
    auto *pos_temp = new arma::uvec(eigen_sys.frame_trans_pos);
    auto *phase_temp = new arma::cx_cube(residual_phase_propagator_time_dep);
    return std::pair<arma::uvec *, arma::cx_cube *>(pos_temp, phase_temp);
}

arma::cx_mat HamiltonianCompiler::cal_residual_phase(double start_time, double dt,
                                                     arma::vec Eeig, arma::vec Eeig_pre,
                                                     arma::cx_mat U, arma::cx_mat U_pre) {

    arma::cx_mat  rot_phase = arma::cx_mat(system_dim,system_dim).fill(0);
    arma::dvec lvl_diff = Eeig-Eeig_pre;
    rot_phase = - start_time * arma::cx_double(1,0)  * arma::diagmat(lvl_diff);
//    std::cout << "Time lapse since last trans:\n" << start_time << std::endl;
//    std::cout << "Eig level diff:\n" << lvl_diff << std::endl;

    // Geometrical phase
    // phi_g = exp(i*E(ti)*ti)*(I - Ud^(ti-dt)^(-1)*Ud(ti))*exp(-i*E(ti)*ti)
    arma::cx_mat rot_frame_phase = arma::expmat(start_time * arma::cx_double(0,1) *  arma::diagmat(Eeig));
    arma::cx_mat rot_frame_phase_inv = arma::expmat(-start_time * arma::cx_double(0,1) *  arma::diagmat(Eeig));
    arma::cx_mat geometrical_phase = arma::cx_mat(system_dim,system_dim).fill(0);
    geometrical_phase = - arma::cx_double(0,1) * rot_frame_phase * (arma::eye(system_dim,system_dim) - arma::inv(U)*U_pre) * rot_frame_phase_inv;
//    std::cout << "Geometrical Residual Phase:\n" << geometrical_phase << std::endl;

    return geometrical_phase+rot_phase;
}
