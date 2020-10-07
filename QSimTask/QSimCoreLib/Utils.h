//
// Created by Rocky Su on 20/9/20.
//
#include <armadillo>
#include <cmath>

namespace qmt{

    const double pi = 3.14159265359;       //pi
    const double h = 6.626e-34;            //Planck constant
    const double h_bar = 6.626e-34/2/pi;   //Reduced Planck constant
    const double e = 1.6e-19;              //Elementary charge
    const double g = 2.01;                 //Electron g-factor
    const double mu_B = 9.27e-24;          //Bohr Magneton
    const double k = 1.38e-23;             //Boltzmann constant

    arma::cx_mat22 pauli_x();
    arma::cx_mat22 pauli_y();
    arma::cx_mat22 pauli_z();

    arma::vec heaviside(int x_0, int length); // HeavySide function.

    arma::cx_mat spinorDecoder(std::string spinorStr);
    arma::cx_mat spinorExpressionDecoder(std::string expression);

    arma::vec generate_filter_func_with_seed_vec(arma::vec seed_vec, int time_steps);
};

std::vector<std::string> str_split(std::string s, char delimiter);

/* Load matrix from config string:
 * e.x.
 * 1 "XI"
 *   XI will be automatically decoded by spinorDecoder()
 * 2 "user_defined"
 *   Matrix will be loaded from external file with csv format.
 * */
arma::cx_mat load_matrix_from_config_str(std::string mat_string);

/*
 * Decode the sequence string to the elementary gate&param pair vector
 *
 * */
std::vector<std::pair<std::string,std::string>> symbolic_sequence_decoder(std::string seq_expression);