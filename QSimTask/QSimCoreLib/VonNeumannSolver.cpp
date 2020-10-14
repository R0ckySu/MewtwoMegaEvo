//
// Created by Rocky Su on 16/9/20.
//

#include "VonNeumannSolver.h"
#include <random>
#include <complex>

VonNeumannSolver::VonNeumannSolver() {

}

VonNeumannSolver::~VonNeumannSolver() {
    arma::cx_cube().swap(propagator);
    arma::cx_cube().swap(propagator_dagger);
}

void VonNeumannSolver::calculate_evolution() {
    if (!verify_inputdata()) {
        std::cout << "VonNeumannSolver:" << "Please check the input data dimensions!" << std::endl;
        return;
    }

    arma::cx_cube hamiltonian_all = *ctrl_hamiltonian_time_dep + *noise_hamiltonian_time_dep;
    int num_steps = hamiltonian_all.n_slices;
    int h_size = hamiltonian_all.n_rows;
    int num_rhos = rho0_multi->size();

    std::vector<arma::cx_cube> rho_t_multi_temp = std::vector<arma::cx_cube>(num_rhos);
    for (int i = 0; i < num_rhos; ++i) {
        rho_t_multi_temp.at(i) = arma::cx_cube(h_size,h_size,num_steps+1).fill(0);
    }

    if (will_record_unitary) {
        //Initialise unitary matrices
        propagator = arma::cx_cube(h_size,h_size,num_steps+1).fill(0);
        propagator.slice(0) = arma::cx_mat(arma::eye(h_size,h_size),arma::zeros(h_size,h_size));
        propagator_dagger = arma::cx_cube(h_size,h_size,num_steps+1).fill(0);
        propagator_dagger.slice(0) = arma::cx_mat(arma::eye(h_size,h_size),arma::zeros(h_size,h_size));
    }

    arma::cx_cube exp_hamiltonians = arma::cx_cube(h_size,h_size,num_steps).fill(0);
    std::complex<double> ii = std::complex<double>(0,1);
    for (int i = 0; i < hamiltonian_all.n_slices; ++i) {
        exp_hamiltonians.slice(i) = qmt::custom_matrix_exp(ii * hamiltonian_all.slice(i));
        if (will_record_unitary) {
            propagator.slice(i+1) = exp_hamiltonians.slice(i) * propagator.slice(i);
            propagator_dagger.slice(i+1) = propagator_dagger.slice(i)*exp_hamiltonians.slice(i).t();
        }
    }

    for (int k = 0; k < num_rhos; ++k) {
        for (int j = 0; j < num_steps + 1; ++j) {
            if (j==0) {
                rho_t_multi_temp.at(k).slice(j) = rho0_multi->at(k).mat;
            } else {
                rho_t_multi_temp.at(k).slice(j) = exp_hamiltonians.slice(j-1) * rho_t_multi_temp.at(k).slice(j-1) * exp_hamiltonians.slice(j-1).t();
            }
        }
    }

    std::cout << "VonNeumann: finished for one shot" << std::endl;

    #pragma omp critical
    {
        for (int i = 0; i < num_rhos; ++i) {
            rho_t_multi->at(i) = rho_t_multi->at(i) + rho_t_multi_temp.at(i)/total_repeat_num;
        }
        std::cout << "VonNeumann: finished joining data" << std::endl;
    };
}

/**********************************************************************************************************************/

bool VonNeumannSolver::verify_inputdata() {
    bool verified = false;

    bool hamiltonian_size_check = (ctrl_hamiltonian_time_dep->n_slices == noise_hamiltonian_time_dep->n_slices) &&
                                    (ctrl_hamiltonian_time_dep->n_cols == noise_hamiltonian_time_dep->n_cols) &&
                                    (ctrl_hamiltonian_time_dep->n_rows == noise_hamiltonian_time_dep->n_rows);

    bool initstate_size_check = rho_t_multi->size() == rho0_multi->size();

    bool length_check = rho_t_multi->at(0).n_slices == (ctrl_hamiltonian_time_dep->n_slices+1);

    verified = hamiltonian_size_check && initstate_size_check && length_check;

    return verified;
}
