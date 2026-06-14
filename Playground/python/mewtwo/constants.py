"""Pauli matrices, singlet-triplet basis, and label arrays.

Port of Constants.m — provides the quantum operator definitions used
throughout MewtwoMegaEvo post-processing.
"""

import numpy as np

# =============================================================================
# 1-Qubit Pauli Matrices
# =============================================================================

I = np.eye(2, dtype=complex)
X = np.array([[0.0, 1.0], [1.0, 0.0]], dtype=complex)
Y = np.array([[0.0, -1j], [1j, 0.0]], dtype=complex)
Z = np.array([[1.0, 0.0], [0.0, -1.0]], dtype=complex)

Pauli1: dict[str, np.ndarray] = {
    "I": I,
    "X": X,
    "Y": Y,
    "Z": Z,
}

# =============================================================================
# 2-Qubit Pauli Matrices (Kronecker products)
# =============================================================================

Pauli2: dict[str, np.ndarray] = {
    "II": np.kron(I, I),
    "IX": np.kron(I, X),
    "IY": np.kron(I, Y),
    "IZ": np.kron(I, Z),
    "XI": np.kron(X, I),
    "XX": np.kron(X, X),
    "XY": np.kron(X, Y),
    "XZ": np.kron(X, Z),
    "YI": np.kron(Y, I),
    "YX": np.kron(Y, X),
    "YY": np.kron(Y, Y),
    "YZ": np.kron(Y, Z),
    "ZI": np.kron(Z, I),
    "ZX": np.kron(Z, X),
    "ZY": np.kron(Z, Y),
    "ZZ": np.kron(Z, Z),
}

# =============================================================================
# Singlet-Triplet Basis (2-electron system)
# =============================================================================

_S0_vec = np.array([0.0, 1.0 / np.sqrt(2), -1.0 / np.sqrt(2), 0.0], dtype=complex)
_T0_vec = np.array([0.0, 1.0 / np.sqrt(2), 1.0 / np.sqrt(2), 0.0], dtype=complex)
_Tp_vec = np.array([1.0, 0.0, 0.0, 0.0], dtype=complex)
_Tm_vec = np.array([0.0, 0.0, 0.0, 1.0], dtype=complex)

# Density matrices: |ψ⟩⟨ψ|
ST: dict[str, np.ndarray] = {
    "S0": np.outer(_S0_vec, _S0_vec.conj()),
    "T0": np.outer(_T0_vec, _T0_vec.conj()),
    "Tp": np.outer(_Tp_vec, _Tp_vec.conj()),
    "Tm": np.outer(_Tm_vec, _Tm_vec.conj()),
}

# State vectors
STvec: dict[str, np.ndarray] = {
    "S0": _S0_vec,
    "T0": _T0_vec,
    "Tp": _Tp_vec,
    "Tm": _Tm_vec,
}

# =============================================================================
# Label Arrays
# =============================================================================

PauliSTLabels: list[str] = ["S0", "T0", "Tp", "Tm"]
Pauli1Labels: list[str] = ["I", "X", "Y", "Z"]
Pauli2Labels: list[str] = [
    "II", "IX", "IY", "IZ",
    "XI", "XX", "XY", "XZ",
    "YI", "YX", "YY", "YZ",
    "ZI", "ZX", "ZY", "ZZ",
]
