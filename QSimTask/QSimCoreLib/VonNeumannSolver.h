//
// Created by Rocky Su on 16/9/20.
//

#include <armadillo>
#include <iostream>
#include "Utils.h"

/*
 * VonNeumannSolver
 *
 * Solve for time evolution of density matrice by von-Neumann Equation:
 *      \rho(t+dt) = U(t+dt)*rho(t)*U(t+dt)^dagger
 * where U(t+dt)
 *      U(t+dt) = U(t) * exp(H(t)*dt)
 *
 * Input:
 * 1 Initial density matrix rho_0
 * 2 Time dependent Hamiltonian. H(t)
 *
 * Output:
 * 1 Unitary Matrices (Optional)
 * 2 Time dependent density matrix rho(t)
 *
 * */

//TODO: log control

class VonNeumannSolver {
public:
    bool will_record_propagator = false;
    int total_repeat_num = 1; //Do not use it for iteration control!!!

    arma::cx_cube propagator;
    arma::cx_cube propagator_dagger;

    std::vector<symbolic_matrix> *rho0_multi;
    std::vector<arma::cx_cube> *rho_t_multi;
    arma::cx_cube *ctrl_hamiltonian_time_dep;
    arma::cx_cube *noise_hamiltonian_time_dep;

    VonNeumannSolver();
    ~VonNeumannSolver();
    void calculate_evolution();
    arma::cx_cube get_propagator_time_evo();
    arma::cx_mat get_propagator_end();
    arma::cx_cube get_propagator_dagger_time_evo();

private:
    bool verify_inputdata();
};


