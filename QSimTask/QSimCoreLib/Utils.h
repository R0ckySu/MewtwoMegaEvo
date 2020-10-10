//
// Created by Rocky Su on 20/9/20.
//

#ifndef MEWTWOMEGAEVO_UTILS_H
#define MEWTWOMEGAEVO_UTILS_H

#include <armadillo>
#include <cmath>
#include <iomanip>

struct symbolic_matrix {
    std::string symbol_name;
    arma::cx_mat mat;
    void load_from_symbol(std::string _symbol_name, std::string config_path);
};

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

    arma::cx_mat spinorDecoder(std::string spinorStr);
    arma::cx_mat spinorExpressionDecoder(std::string expression);

/*
 * Matrix exponent approximated by Pade approximation
 * */
    arma::cx_mat custom_matrix_exp(arma::cx_mat input_matrix);
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
 * Get current time in the 'YYYYMMDDHHMMSSSS' format
 * */
std::string get_time_stamp_str();

/*
 * Transform double to fixed precision in scientific format
 * */
std::string double_to_fixprecision_str(double num, int percision);

/*
 * Symbolic Sequence string parser
 * - Sequence decomposition rule
 *  |- Sub sequence wrapped by square braket "[]"
 *  |- "[]^n" will repeat the sub sequence for n times.
 *  |- Each symbolic gate in the sequence is separated by '-'
 * - E.x.
 *  |-Sequence_string: F(T/4)-[U(0,pi,X)-U(0,pi,Y)]^2-U(0,pi,X)-F(T/4)
 *  |-Result: F(T/4), U(0,pi,X), U(0,pi,Y), U(0,pi,X), U(0,pi,Y), U(0,pi,X), F(T/4)
 *  -- Where, gate symbols are decomposed to tag-param pair, for e.x., F(T/4) will be decomposed to "F" : "T/4"
 * */
std::vector<std::pair<std::string, std::string>> symbolic_sequence_str_parser (std::string sequence_str);

std::pair<std::string, std::string> decompose_gate_string_to_tag_param_pair(std::string gate_str);

#endif //MEWTWOMEGAEVO_UTILS_H