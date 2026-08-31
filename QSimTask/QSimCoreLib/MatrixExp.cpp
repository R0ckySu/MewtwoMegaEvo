//
// Created by Claude on 31/8/26.
//

#include "MatrixExp.h"
#include "Utils.h"
#include <complex>
#include <stdexcept>
#include <vector>

namespace qmt {

    static const std::complex<double> exp_i(0, 1);

    ExpMethod exp_method_from_string(const std::string &name) {
        if (name == "taylor_legacy") return ExpMethod::TaylorLegacy;
        if (name == "pade") return ExpMethod::Pade;
        if (name == "chebyshev") return ExpMethod::Chebyshev;
        if (name == "diagonalization") return ExpMethod::Diagonalization;
        throw std::invalid_argument(
                "Unknown matrix_exp_method: '" + name +
                "'. Expected one of: taylor_legacy, pade, chebyshev, diagonalization");
    }

    static bool is_hermitian(const arma::cx_mat &H) {
        double h_norm = arma::norm(H, "fro");
        if (h_norm == 0.0) return true;
        return arma::norm(H - H.t(), "fro") <= 1e-12 * h_norm;
    }

    /*
     * [13/13] diagonal Pade with scaling-and-squaring on A = i*H (Higham,
     * SIAM J. Matrix Anal. Appl. 26, 1179 (2005)). For skew-Hermitian A the
     * diagonal Pade approximant is exactly unitary (a Cayley-type transform).
     * */
    static arma::cx_mat pade13_exp(const arma::cx_mat &H) {
        static const double b[14] = {
                64764752532480000.0, 32382376266240000.0, 7771770303897600.0,
                1187353796428800.0, 129060195264000.0, 10559470521600.0,
                670442572800.0, 33522128640.0, 1323241920.0, 40840800.0,
                960960.0, 16380.0, 182.0, 1.0};
        static const double theta13 = 5.371920351148152;

        arma::cx_mat A = exp_i * H;
        const double norm1 = arma::norm(A, 1);
        int s = 0;
        if (norm1 > theta13) {
            s = int(std::ceil(std::log2(norm1 / theta13)));
            A /= std::pow(2.0, s);
        }

        const arma::uword n = A.n_rows;
        const arma::cx_mat I = arma::cx_mat(arma::eye(n, n), arma::zeros(n, n));
        const arma::cx_mat A2 = A * A;
        const arma::cx_mat A4 = A2 * A2;
        const arma::cx_mat A6 = A2 * A4;

        arma::cx_mat U = A * (A6 * (b[13] * A6 + b[11] * A4 + b[9] * A2) +
                              b[7] * A6 + b[5] * A4 + b[3] * A2 + b[1] * I);
        arma::cx_mat V = A6 * (b[12] * A6 + b[10] * A4 + b[8] * A2) +
                         b[6] * A6 + b[4] * A4 + b[2] * A2 + b[0] * I;

        arma::cx_mat F = arma::solve(V - U, V + U);
        for (int i = 0; i < s; ++i) { F = F * F; }
        return F;
    }

    /*
     * Bessel functions J_0..J_K at x>0 by Miller's downward recurrence,
     * normalized with J_0 + 2*sum_{k even} J_k = 1.
     * */
    static std::vector<double> bessel_j_list(double x, int K) {
        int M = K + 15 + int(std::sqrt(40.0 * K));
        if (M % 2 != 0) { M++; }
        std::vector<double> f(M + 2, 0.0);
        f[M] = 1e-300;
        double norm_sum = 0.0;
        for (int k = M; k > 0; --k) {
            f[k - 1] = (2.0 * k / x) * f[k] - f[k + 1];
            if (std::abs(f[k - 1]) > 1e250) {
                for (int j = k - 1; j <= M + 1; ++j) { f[j] *= 1e-250; }
                norm_sum *= 1e-250;
            }
            if ((k - 1) > 0 && (k - 1) % 2 == 0) { norm_sum += 2.0 * f[k - 1]; }
        }
        norm_sum += f[0];
        std::vector<double> J(K + 1);
        for (int k = 0; k <= K; ++k) { J[k] = f[k] / norm_sum; }
        return J;
    }

    /*
     * Chebyshev expansion of exp(i*H) for Hermitian H (Tal-Ezer & Kosloff,
     * J. Chem. Phys. 81, 3967 (1984); Blanes, Casas & Escorihuela-Tomas,
     * Math. Comput. Simul. 194, 383 (2022)).
     *
     * Shift-and-scale H = c*I + alpha*X with spec(X) in [-1,1] (Gershgorin
     * bounds), then by the Jacobi-Anger identity
     *      exp(i*alpha*X) = J_0(alpha)*I + 2*sum_k i^k J_k(alpha) T_k(X),
     * truncated where |J_K(alpha)| <= (alpha/2)^K / K! < eps, evaluated by
     * matrix Clenshaw recurrence. exp(i*H) = exp(i*c) * exp(i*alpha*X).
     * */
    static arma::cx_mat chebyshev_exp(const arma::cx_mat &H) {
        const arma::uword n = H.n_rows;

        // Gershgorin spectral bounds
        double lambda_min = arma::datum::inf;
        double lambda_max = -arma::datum::inf;
        for (arma::uword i = 0; i < n; ++i) {
            double radius = 0.0;
            for (arma::uword j = 0; j < n; ++j) {
                if (j != i) { radius += std::abs(H(i, j)); }
            }
            double center = H(i, i).real();
            lambda_min = std::min(lambda_min, center - radius);
            lambda_max = std::max(lambda_max, center + radius);
        }

        const double c = 0.5 * (lambda_max + lambda_min);
        const double alpha = 0.5 * (lambda_max - lambda_min);
        const arma::cx_mat I = arma::cx_mat(arma::eye(n, n), arma::zeros(n, n));

        // Truncation degree from |J_K(alpha)| <= (alpha/2)^K / K!
        int K = 1;
        double term = 0.5 * alpha;
        while (term > 1e-16 && K < 1000) {
            K++;
            term *= 0.5 * alpha / K;
        }
        if (K >= 1000) { return pade13_exp(H); }

        const std::complex<double> phase = std::exp(exp_i * c);

        if (alpha < 1e-3) {
            // Spectrum nearly a point: plain Taylor on the shifted matrix is
            // exact to eps at this degree and avoids Miller-recurrence overflow.
            arma::cx_mat A = exp_i * (H - c * I);
            arma::cx_mat power = I;
            arma::cx_mat F = I;
            double factorial = 1.0;
            for (int k = 1; k <= K; ++k) {
                factorial *= k;
                power = power * A;
                F += power / factorial;
            }
            return phase * F;
        }

        const arma::cx_mat X = (H - c * I) / alpha;
        std::vector<double> J = bessel_j_list(alpha, K);

        // Clenshaw: b_k = c_k + 2*X*b_{k+1} - b_{k+2}, f = c_0 + X*b_1 - b_2
        std::vector<std::complex<double>> coeff(K + 1);
        coeff[0] = J[0];
        std::complex<double> ik(1, 0);
        for (int k = 1; k <= K; ++k) {
            ik *= exp_i;
            coeff[k] = 2.0 * ik * J[k];
        }

        arma::cx_mat b_kp1(n, n, arma::fill::zeros);
        arma::cx_mat b_kp2(n, n, arma::fill::zeros);
        for (int k = K; k >= 1; --k) {
            arma::cx_mat b_k = coeff[k] * I + 2.0 * (X * b_kp1) - b_kp2;
            b_kp2 = std::move(b_kp1);
            b_kp1 = std::move(b_k);
        }
        return phase * (coeff[0] * I + X * b_kp1 - b_kp2);
    }

    static arma::cx_mat diagonalization_exp(const arma::cx_mat &H) {
        arma::vec eigval;
        arma::cx_mat eigvec;
        arma::eig_sym(eigval, eigvec, H);
        return eigvec * arma::diagmat(arma::exp(exp_i * arma::conv_to<arma::cx_vec>::from(eigval))) * eigvec.t();
    }

    arma::cx_mat unitary_exp(const arma::cx_mat &H, ExpMethod method) {
        switch (method) {
            case ExpMethod::TaylorLegacy:
                return custom_matrix_exp(exp_i * H);
            case ExpMethod::Pade:
                return pade13_exp(H);
            case ExpMethod::Chebyshev:
                if (!is_hermitian(H)) { return pade13_exp(H); }
                return chebyshev_exp(H);
            case ExpMethod::Diagonalization:
                if (!is_hermitian(H)) { return pade13_exp(H); }
                return diagonalization_exp(H);
        }
        return custom_matrix_exp(exp_i * H);
    }
}
