"""Check the Python reference environment with a small known-solution system."""

import platform
import sys

import numpy as np
import scipy
from scipy.sparse import csr_matrix
from scipy.sparse.linalg import spsolve


def main():
    print(f"Python: {sys.version.split()[0]} ({platform.system()})")
    print(f"NumPy: {np.__version__}")
    print(f"SciPy: {scipy.__version__}")

    matrix = np.array([[2.0, -1.0], [-1.0, 2.0]])
    sparse_matrix = csr_matrix(matrix)
    expected_solution = np.array([1.0, 2.0])
    # Independently computed: [[2, -1], [-1, 2]] @ [1, 2] = [0, 3].
    rhs = np.array([0.0, 3.0])
    np.testing.assert_allclose(matrix @ expected_solution, rhs, rtol=1e-12, atol=1e-12)
    np.testing.assert_allclose(
        sparse_matrix @ expected_solution, rhs, rtol=1e-12, atol=1e-12
    )

    dense_solution = np.linalg.solve(matrix, rhs)
    sparse_solution = spsolve(sparse_matrix, rhs)
    for solution in (dense_solution, sparse_solution):
        np.testing.assert_allclose(solution, expected_solution, rtol=1e-12, atol=1e-12)
        np.testing.assert_allclose(rhs - matrix @ solution, 0.0, rtol=0.0, atol=1e-12)

    print("Python reference environment: PASS")


if __name__ == "__main__":
    main()
