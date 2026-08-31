//
// Created by Claude on 31/8/26.
//

#ifndef MEWTWOMEGAEVO_MATRIXEXP_H
#define MEWTWOMEGAEVO_MATRIXEXP_H

#include <armadillo>
#include <string>

/*
 * Configurable matrix exponentiation methods for the propagator U = exp(i*H).
 *
 * Methods (config key "matrix_exp_method" in sim_config.json):
 *  - "taylor_legacy":    original qmt::custom_matrix_exp (scaling + degree-9 Taylor).
 *  - "pade":             [13/13] Pade with scaling-and-squaring (Higham 2005).
 *                        Works for any matrix; exactly unitary for Hermitian H.
 *  - "chebyshev":        Chebyshev expansion of exp(i*H) on the spectral interval
 *                        (Tal-Ezer & Kosloff 1984; Blanes et al. 2022). Requires
 *                        Hermitian H; falls back to "pade" otherwise.
 *  - "diagonalization":  exact, via eig_sym: exp(i*H) = V diag(exp(i*lambda)) V^dagger.
 *                        Requires Hermitian H; falls back to "pade" otherwise.
 * */

namespace qmt {

    enum class ExpMethod {
        TaylorLegacy,
        Pade,
        Chebyshev,
        Diagonalization
    };

    // Throws std::invalid_argument on unknown name.
    ExpMethod exp_method_from_string(const std::string &name);

    // Computes exp(i*H) with the selected method. H must be square;
    // Hermitian-only methods verify hermiticity and fall back to Pade.
    arma::cx_mat unitary_exp(const arma::cx_mat &H, ExpMethod method);
}

#endif //MEWTWOMEGAEVO_MATRIXEXP_H
