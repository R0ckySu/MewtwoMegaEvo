//
// Created by Rocky Su on 16/9/20.
//

#include <armadillo>
#include <iostream>

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

class VonNeumannSolver {
public:
    bool will_record_unitary = false;
    int hilberspace_size;

    std::vector<arma::cx_mat> *rho0_multi;
    std::vector<arma::cx_cube> *rho_t_multi;
    arma::cx_cube propagator;
    arma::cx_cube propagator_dagger;

    arma::cx_cube *ctrl_hamiltonian_time_dep;
    arma::cx_cube *noise_hamiltonian_time_dep;

    VonNeumannSolver();
    ~VonNeumannSolver();
    void calculate_evolution();

private:
    arma::cx_mat custom_matrix_exp(arma::cx_mat input_matrix);
};


