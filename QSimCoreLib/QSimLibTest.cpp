//
// Created by Rocky Su on 16/9/20.
//
#include <iostream>
#include <armadillo>
#include "Sequence.h"
#include "VonNeumannSolver.h"
#include "Hamiltonian.h"
#include "Utils.h"

int main() {
//    Sequence seq = Sequence();
//    seq.test();

    Sequence seq = Sequence();
    int total_num_steps = 40;
    double step_size = 1e-7;

    seq.total_num_steps = total_num_steps;
    seq.step_size = step_size;

    MW_Hamiltonian X1 = MW_Hamiltonian();
    X1.tag = "X1";
    X1.h_mat = qmt::spinorDecoder("XI");
    X1.amplitude = 1.5e1;
    X1.phase = M_PI/2;
    X1.freq = 1e4;
    X1.num_of_steps = total_num_steps;
    X1.step_size = step_size;

    Gate gate1 = Gate();
    gate1.tag = "G1";
    gate1.set_pulse_width(1e-6);
    gate1.step_size = step_size;
    seq.append_gate(gate1);
    seq.append_gate(gate1);

    Gate gate2 = Gate();
    gate2.tag = "G2";
    gate2.set_pulse_width(3e-6);
    gate2.step_size = step_size;
//    seq.append_gate(gate2);

    seq.generate_switching_sig();
    X1.switching_signal = seq.gate_switching_map["G1"];
    std::cout << "SwitchingSig" << X1.switching_signal << std::endl;

    std::vector<arma::cx_mat> rho0s = std::vector<arma::cx_mat>(2);
    rho0s.at(0) = qmt::spinorDecoder("IY");
    rho0s.at(1) = qmt::spinorDecoder("YI");
    std::vector<arma::cx_cube> rho_ts = std::vector<arma::cx_cube>(2);
    rho_ts.at(0) = arma::cx_cube(4,4,total_num_steps+1);
    rho_ts.at(1) = arma::cx_cube(4,4,total_num_steps+1);
    arma::cx_cube ctrl_h = arma::cx_cube(4,4,total_num_steps);
    arma::cx_cube noise_h = arma::cx_cube(4,4,total_num_steps);

    X1.load_waveform();
    std::cout << "Waveform" << X1.wave_form << std::endl;
    X1.fetch_H(&ctrl_h);
//    std::cout << "HamiltonianCtrl" << ctrl_h << std::endl;

    VonNeumannSolver solver_obj = VonNeumannSolver();
    solver_obj.rho0_multi = &rho0s;
    solver_obj.rho_t_multi = &rho_ts;
    solver_obj.ctrl_hamiltonian_time_dep = &ctrl_h;
    solver_obj.noise_hamiltonian_time_dep = &noise_h;

    solver_obj.calculate_evolution();

    std::cout << rho_ts.at(0) << std::endl;
    return 0;
}