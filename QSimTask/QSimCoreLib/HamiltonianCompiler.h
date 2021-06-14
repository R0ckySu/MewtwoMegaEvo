//
// Created by Rocky Su on 14/6/21.
//

#ifndef MEWTWOMEGAEVO_HAMILTONIANCOMPILER_H
#define MEWTWOMEGAEVO_HAMILTONIANCOMPILER_H

#include "SimPrototypes.h"
#include "Sequence.h"

class HamiltonianCompiler {
public:
    HamiltonianCompiler();

    int system_dim = 0;
    int total_num_steps = 0;
    double step_size = 1;

    HamiltonianPrototypes h_prototype_ptr;

    arma::cx_cube * ctrl_hamiltonian_time_dep;

    void load_ctrl_signals_from_sequence(Sequence &seq, GatePrototypes g_proto);

    arma::cx_cube * compile_ctrl_hamiltonian();
    arma::cx_cube * compile_noise_hamiltonian(int noise_idx, std::vector<double> random_start_pos_factor);
};


#endif //MEWTWOMEGAEVO_HAMILTONIANCOMPILER_H
