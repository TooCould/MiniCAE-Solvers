"""Validate actual C++ CSR conversion and operations against independent SciPy inputs."""
import argparse
import csv
import io
from pathlib import Path
import subprocess
import sys

import numpy as np
import scipy
from scipy.sparse import coo_matrix


def cases():
    yield "empty", (0, 0), [], 0.0
    yield "zero_rows", (0, 4), [], 0.0
    yield "zero_cols", (4, 0), [], 0.0
    yield "zero_square", (5, 5), [], 0.0
    yield "unsorted_tall", (7, 3), [(6, 2, 2.5), (0, 1, -3), (3, 2, 0.25)], 0.0
    yield "wide_missing_diagonal", (2, 7), [(1, 6, -2), (0, 0, 4)], 0.0
    yield "duplicates", (3, 5), [(2, 4, 2), (0, 1, 1), (0, 1, 3)], 0.0
    yield "cancellation", (3, 3), [(1, 2, 4), (1, 2, -4), (2, 2, 0)], 0.0
    yield "intermediate_zero", (3, 3), [(1, 2, 4), (1, 2, -4), (1, 2, 2)], 0.0
    yield "explicit_zeros", (2, 3), [(0, 0, 0), (1, 1, -0.0), (1, 2, -0.25)], 0.0
    yield "symmetric", (3, 3), [(2, 0, -0.5), (0, 2, -0.5), (1, 1, 2)], 0.0
    yield "bottom_asymmetric", (4, 4), [(2, 3, 2)], 0.0
    yield "missing_within_atol", (2, 2), [(0, 1, 0.25)], 0.5
    yield "missing_at_boundary", (2, 2), [(0, 1, 0.25)], 0.25
    yield "missing_beyond_atol", (2, 2), [(0, 1, 0.25)], 0.125
    yield "difference_at_boundary", (2, 2), [(0, 1, 1), (1, 0, 1.5)], 0.5
    yield "opposite_small_values", (2, 2), [(0, 1, 0.75), (1, 0, -0.75)], 1.0
    yield "sparse_many_rows", (50000, 7), [(49999, 6, 2), (100, 2, -4)], 0.0
    rng = np.random.default_rng(20261110)
    for rows, cols in [(1, 1), (1, 11), (13, 1), (17, 23), (25, 25)]:
        count = 500
        # Binary fractions isolate structural correctness from summation-order differences.
        entries = [(int(r), int(c), float(v) / 4) for r, c, v in zip(
            rng.integers(rows, size=count), rng.integers(cols, size=count),
            rng.integers(-16, 17, size=count))]
        yield f"random_{rows}_{cols}", (rows, cols), entries, 0.25
    for n in [1, 7, 19]:
        entries = []
        for i in range(n):
            for j in range(i, n):
                value = float(rng.integers(-8, 9)) / 4
                entries.append((i, j, value))
                if i != j:
                    entries.append((j, i, value))
        rng.shuffle(entries)
        yield f"random_symmetric_{n}", (n, n), entries, 0.0


INTEGER_FIELDS = {"rows", "cols", "nnz", "row_ptr", "col_indices", "symmetric"}
FIELDS = INTEGER_FIELDS | {"values", "matvec", "diagonal", "norm_inf"}


def read_cpp(text):
    reader = csv.DictReader(io.StringIO(text))
    if reader.fieldnames != ["type", "index", "value"]:
        raise ValueError("Unexpected CSV header")
    actual = {key: [] for key in FIELDS}
    for row in reader:
        key = row["type"]
        if key not in actual or int(row["index"]) != len(actual[key]):
            raise ValueError("Unknown field or nonsequential output index")
        actual[key].append(int(row["value"]) if key in INTEGER_FIELDS else float(row["value"]))
    return actual


def reference(shape, entries, x, atol):
    rows = np.array([r for r, _, _ in entries], dtype=np.int64)
    cols = np.array([c for _, c, _ in entries], dtype=np.int64)
    values = np.array([v for _, _, v in entries], dtype=np.float64)
    a = coo_matrix((values, (rows, cols)), shape=shape).tocsr()
    a.sum_duplicates()
    a.eliminate_zeros()
    a.sort_indices()
    row_sums = np.asarray(abs(a).sum(axis=1)).ravel()
    symmetric = False
    if shape[0] == shape[1]:
        difference = a - a.T
        symmetric = bool(np.all(np.abs(difference.data) <= atol))
    return {"rows": [shape[0]], "cols": [shape[1]], "nnz": [a.nnz],
            "row_ptr": a.indptr.tolist(), "col_indices": a.indices.tolist(),
            "values": a.data.tolist(), "matvec": (a @ x).tolist(),
            "diagonal": a.diagonal().tolist(),
            "norm_inf": [float(np.max(row_sums, initial=0.0))],
            "symmetric": [int(symmetric)]}


def write_reference(path, expected):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(["type", "index", "value"])
        for key, values in expected.items():
            writer.writerows((key, i, value) for i, value in enumerate(values))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--output-dir", type=Path, default=Path("out/validation/compressed_sparse_row"))
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    summary = args.output_dir / "summary.csv"
    summary.unlink(missing_ok=True)
    executable = args.exe.resolve(strict=True)
    print(f"Python {sys.version.split()[0]}; NumPy {np.__version__}; SciPy {scipy.__version__}")
    results = []
    for name, shape, entries, atol in cases():
        x = ((np.arange(shape[1], dtype=np.float64) % 9) - 4) / 4
        input_text = f"{shape[0]} {shape[1]} {len(entries)} {atol:.17g}\n"
        input_text += "".join(f"{r} {c} {v:.17g}\n" for r, c, v in entries)
        input_text += " ".join(f"{v:.17g}" for v in x) + "\n"
        (args.output_dir / f"{name}_input.txt").write_text(input_text, encoding="utf-8")
        run = subprocess.run([str(executable)], input=input_text, text=True, encoding="utf-8",
                             capture_output=True, check=True, timeout=30)
        (args.output_dir / f"{name}_cpp.csv").write_text(run.stdout, encoding="utf-8")
        actual = read_cpp(run.stdout)
        expected = reference(shape, entries, x, atol)
        write_reference(args.output_dir / f"{name}_scipy.csv", expected)
        for key in FIELDS:
            if len(actual[key]) != len(expected[key]):
                raise ValueError(f"{name}: {key} output length mismatch")
            if key in INTEGER_FIELDS:
                if actual[key] != expected[key]:
                    raise ValueError(f"{name}: {key} integer/structure mismatch")
            else:
                np.testing.assert_allclose(actual[key], expected[key], rtol=1e-12, atol=1e-12,
                                           equal_nan=False, err_msg=f"{name}: {key}")
        results.append((name, shape[0], shape[1], expected["nnz"][0], "PASS"))
        print(f"{name}: PASS")
    with summary.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(["case", "rows", "cols", "nnz", "status"])
        writer.writerows(results)
    (args.output_dir / "environment.txt").write_text(
        f"Python: {sys.version}\nNumPy: {np.__version__}\nSciPy: {scipy.__version__}\n"
        f"Executable: {executable}\n", encoding="utf-8")
    print(f"CSR SciPy validation: PASS ({len(results)} cases)")


if __name__ == "__main__":
    main()
