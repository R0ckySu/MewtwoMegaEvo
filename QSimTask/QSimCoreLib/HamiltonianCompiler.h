//
// Created by Rocky Su on 14/6/21.
//

#ifndef MEWTWOMEGAEVO_HAMILTONIANCOMPILER_H
#define MEWTWOMEGAEVO_HAMILTONIANCOMPILER_H

#include "SimPrototypes.h"
#include "Sequence.h"

struct EigenSysTimeDepStruct {
    EigenSysTimeDepStruct();
    EigenSysTimeDepStruct(uint num_of_steps, uint sys_size);
    double time_step;
    arma::uvec frame_trans_pos;
    arma::mat eigen_energy_time_dep;
    arma::cx_cube basis_trans_time_dep;

    arma::cx_cube residual_phase_H_time_dep;
    arma::cube eigen_freq_mask_time_dep;
};

class HamiltonianCompiler {
public:
    HamiltonianCompiler();
    ~HamiltonianCompiler();

    bool turn_on_dynamic_frame_trans = true;
    int system_dim = 0;
    int total_num_steps = 0;
    double step_size = 1;
    int log_level_threshold = 4;
    EigenSysTimeDepStruct eigen_sys;
    HamiltonianPrototypes h_prototype_ptr;

    void load_ctrl_signals_from_sequence(Sequence &seq, GatePrototypes g_proto);

//    void generate_dynamic_frame();
    void generate_dynamic_frame2();


    arma::cx_cube * compile_ctrl_hamiltonian();
    arma::cx_cube * compile_noise_hamiltonian(int noise_idx, std::vector<double> random_start_pos_factor);

    std::pair<arma::uvec *, arma::cx_cube *> get_residual_transition_propagator();
    std::pair<arma::uvec *, arma::cx_cube *> get_basis_trans();
private:

    arma::cx_mat cal_residual_phase(double start_time, double dt, arma::vec Eeig, arma::vec Eeig_pre, arma::cx_mat U, arma::cx_mat U_pre);
    arma::cx_cube residual_phase_propagator_time_dep;

    void task_log(std::string message, int log_level) {
        if (log_level < log_level_threshold) {
            std::string log_msg = std::string("HamiltonianCompiler:").append(message);
            std::cout << log_msg << std::endl;
        }
    }
};


#endif //MEWTWOMEGAEVO_HAMILTONIANCOMPILER_H
