"""Compare C++ dense matrix operations with NumPy (use --matmul for A * B)."""

import argparse
import csv
from pathlib import Path
import subprocess
import sys

import numpy as np

from check_vector import read_output


def cases():
    yield "empty", np.zeros((0, 0)), np.zeros(0)
    yield "zero_rows", np.zeros((0, 3)), np.array([1.0, 2.0, 3.0])
    yield "zero_cols", np.zeros((4, 0)), np.zeros(0)
    yield "hand_calculated", np.array([[1.0, 2.0, 3.0], [-4.0, 5.0, 6.0]]), np.array([2.0, -1.0, 0.5])
    yield "tall", np.array([[1.0, -4.0], [2.0, 5.0], [3.0, 6.0]]), np.array([2.0, -1.0])
    yield "identity", np.eye(3), np.array([1.5, -2.0, 0.0])
    yield "zero_matrix", np.zeros((2, 3)), np.array([1.0, -2.0, 3.0])
    rng = np.random.default_rng(20261106)
    for rows, cols in ((1, 1), (1, 5), (5, 1), (3, 7), (7, 3)):
        yield f"random_{rows}x{cols}", rng.normal(size=(rows, cols)), rng.normal(size=cols)


def matmul_cases():
    yield "empty", np.zeros((0, 0)), np.zeros((0, 0))
    yield "zero_rows", np.zeros((0, 3)), np.zeros((3, 2))
    yield "zero_cols", np.zeros((2, 3)), np.zeros((3, 0))
    yield "zero_inner", np.zeros((2, 0)), np.zeros((0, 4))
    a = np.array([[1.0, 2.0, 3.0], [-4.0, 5.0, 6.0]])
    yield "hand_calculated", a, np.array([[7.0, 8.0], [9.0, 10.0], [11.0, 12.0]])
    yield "left_identity", np.eye(2), a
    yield "right_identity", a, np.eye(3)
    yield "zero_matrix", np.zeros((2, 3)), np.ones((3, 4))
    yield "outer", np.array([[1.0], [2.0], [-1.0]]), np.array([[3.0, -2.0]])
    yield "inner", np.array([[1.0, 2.0, 3.0]]), np.array([[4.0], [-1.0], [2.0]])
    rng = np.random.default_rng(20261106)
    for rows, inner, cols in ((1, 1, 1), (2, 3, 4), (5, 2, 7), (3, 6, 2)):
        yield f"random_{rows}x{inner}x{cols}", rng.normal(size=(rows, inner)), rng.normal(size=(inner, cols))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--matmul", action="store_true", help="Validate matrix-matrix multiplication")
    parser.add_argument("--output-dir", type=Path, default=Path("out/validation/dense_matrix"))
    args = parser.parse_args()
    executable = args.exe.resolve(strict=True)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    report = args.output_dir / ("summary_matmul.csv" if args.matmul else "summary.csv")
    if report.exists():
        report.unlink()
    print(f"Python: {sys.version.split()[0]}; NumPy: {np.__version__}")
    print(f"C++ executable: {executable}")
    results = []
    for name, matrix, x in (matmul_cases() if args.matmul else cases()):
        rows, cols = matrix.shape
        input_text = (
            (f"{rows} {cols} {x.shape[1]}\n" if args.matmul else f"{rows} {cols}\n")
            + " ".join(format(value, ".17g") for value in matrix.ravel()) + "\n"
            + " ".join(format(value, ".17g") for value in x.ravel()) + "\n"
        )
        file_name = f"matmul_{name}" if args.matmul else name
        (args.output_dir / f"{file_name}_input.txt").write_text(input_text, encoding="utf-8")
        run = subprocess.run(
            [str(executable)] + (["--matmul"] if args.matmul else []), input=input_text, capture_output=True,
            text=True, encoding="utf-8", timeout=10, check=True,
        )
        (args.output_dir / f"{file_name}_cpp.csv").write_text(run.stdout, encoding="utf-8")
        actual = read_output(run.stdout)
        expected = {
            "rows": np.array([rows]), "cols": np.array([cols]),
            "transpose_rows": np.array([cols]), "transpose_cols": np.array([rows]),
            "product_size": np.array([rows]),
            "transpose": matrix.T.ravel(), "matvec": matrix @ x,
        }
        if args.matmul:
            expected = {
                "rows": np.array([rows]), "cols": np.array([cols]),
                "b_rows": np.array([x.shape[0]]), "b_cols": np.array([x.shape[1]]),
                "product_rows": np.array([rows]), "product_cols": np.array([x.shape[1]]),
                "matmul": (matrix @ x).ravel(),
            }
        if set(actual) - set(expected):
            raise ValueError(f"{name}: unexpected C++ operations")
        with (args.output_dir / f"{file_name}_numpy.csv").open("w", newline="", encoding="utf-8") as file:
            writer = csv.writer(file)
            writer.writerow(["operation", "index", "value"])
            for operation, reference in expected.items():
                observed = np.asarray(actual.get(operation, []), dtype=np.float64)
                if observed.shape != reference.shape:
                    raise AssertionError(f"{name}/{operation}: result size mismatch")
                # The rounding error of a row dot product scales with sum(abs(A*x)).
                # Use it for cancellation near zero, without a fixed absolute floor.
                if operation in ("matvec", "matmul"):
                    error_bound = 1e-12 * (np.abs(matrix) @ np.abs(x)).ravel()
                    if not np.all(np.isfinite(observed)) or not np.all(
                        np.abs(observed - reference) <= error_bound
                    ):
                        raise AssertionError(f"{name}: {operation} differs from NumPy")
                else:
                    np.testing.assert_array_equal(observed, reference, err_msg=f"{name}/{operation}")
                for index, value in enumerate(reference):
                    writer.writerow([operation, index, format(value, ".17g")])
        results.append((name, rows, cols, x.shape[1], "PASS") if args.matmul else (name, rows, cols, "PASS"))
        print(f"PASS: {name}")
    with report.open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["case", "rows", "inner", "cols", "status"] if args.matmul else ["case", "rows", "cols", "status"])
        writer.writerows(results)
    label = "DenseMatrix matmul NumPy validation" if args.matmul else "DenseMatrix NumPy validation"
    print(f"{label}: PASS ({len(results)} cases)")
    print(f"Results: {args.output_dir.resolve()}")


if __name__ == "__main__":
    main()
