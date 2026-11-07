"""Compare actual C++ CoordinateMatrix canonicalization with SciPy COO."""

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
    yield "empty", (0, 0), []
    yield "zero_rows", (0, 4), []
    yield "zero_cols", (3, 0), []
    yield "unique_unsorted", (3, 5), [(2, 4, -2.5), (0, 1, 3.0), (2, 0, 7.0)]
    yield "duplicates", (3, 5), [(1, 2, 1.0), (0, 4, 5.0), (1, 2, 2.0), (1, 2, 3.0)]
    yield "cancellation", (2, 3), [(1, 2, 4.0), (0, 0, 5.0), (1, 2, -4.0), (0, 0, -5.0)]
    yield "intermediate_zero", (2, 3), [(1, 2, 4.0), (1, 2, -4.0), (1, 2, 2.0)]
    yield "explicit_zeros", (2, 3), [(0, 0, 0.0), (1, 1, -0.0), (1, 2, 2.5)]
    yield "empty_rows", (7, 3), [(6, 2, 2.0), (0, 1, 3.0), (6, 2, -1.0)]
    base = 2**53
    yield "large_rows", (base + 4, 2), [(base + 1, 1, 2.0), (base, 1, 3.0)]
    yield "large_cols", (2, base + 4), [(1, base + 1, 2.0), (1, base, 3.0)]
    rng = np.random.default_rng(20261106)
    for count in (1, 30, 1000):
        rows = rng.integers(0, 7, size=count)
        cols = rng.integers(0, 11, size=count)
        # Small integers are exactly representable: isolate grouping correctness
        # from differences in floating-point summation order between libraries.
        values = rng.integers(-8, 9, size=count)
        entries = [(int(r), int(c), float(v)) for r, c, v in zip(rows, cols, values)]
        yield f"random_{count}", (7, 11), entries


def read_cpp(text):
    reader = csv.DictReader(io.StringIO(text))
    if reader.fieldnames != ["type", "row", "col", "value"]:
        raise ValueError("Unexpected C++ CSV header")
    metadata = next(reader, None)
    if metadata is None or metadata["type"] != "shape":
        raise ValueError("Missing C++ shape metadata")
    shape = (int(metadata["row"]), int(metadata["col"]))
    count = int(metadata["value"])
    rows, cols, values = [], [], []
    for entry in reader:
        if entry["type"] != "entry":
            raise ValueError("Unexpected CSV record")
        # Do not parse integer coordinates through float: large indices lose bits.
        rows.append(int(entry["row"]))
        cols.append(int(entry["col"]))
        values.append(float(entry["value"]))
    if len(values) != count:
        raise ValueError("C++ nnz does not match exported entry count")
    return shape, np.asarray(rows, dtype=np.int64), np.asarray(cols, dtype=np.int64), np.asarray(values)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--output-dir", type=Path, default=Path("out/validation/coordinate_matrix"))
    args = parser.parse_args()
    executable = args.exe.resolve(strict=True)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    report = args.output_dir / "summary.csv"
    if report.exists():
        report.unlink()
    print(f"Python: {sys.version.split()[0]}; NumPy: {np.__version__}; SciPy: {scipy.__version__}")
    print(f"C++ executable: {executable}")
    results = []
    for name, shape, entries in cases():
        input_text = f"{shape[0]} {shape[1]} {len(entries)}\n" + "".join(
            f"{row} {col} {value:.17g}\n" for row, col, value in entries
        )
        (args.output_dir / f"{name}_input.txt").write_text(input_text, encoding="utf-8")
        run = subprocess.run(
            [str(executable)], input=input_text, text=True, encoding="utf-8",
            capture_output=True, check=True, timeout=10,
        )
        (args.output_dir / f"{name}_cpp.csv").write_text(run.stdout, encoding="utf-8")
        actual_shape, actual_rows, actual_cols, actual_values = read_cpp(run.stdout)

        # Independent reference constructed from the original input, not C++ output.
        rows = np.array([row for row, _, _ in entries], dtype=np.int64)
        cols = np.array([col for _, col, _ in entries], dtype=np.int64)
        values = np.array([value for _, _, value in entries], dtype=np.float64)
        reference = coo_matrix((values, (rows, cols)), shape=shape)
        reference.sum_duplicates()
        reference.eliminate_zeros()
        order = np.lexsort((reference.col, reference.row))
        expected_rows = reference.row[order]
        expected_cols = reference.col[order]
        expected_values = reference.data[order]

        with (args.output_dir / f"{name}_scipy.csv").open("w", newline="", encoding="utf-8") as file:
            writer = csv.writer(file)
            writer.writerow(["type", "row", "col", "value"])
            writer.writerow(["shape", *shape, reference.nnz])
            for row, col, value in zip(expected_rows, expected_cols, expected_values):
                writer.writerow(["entry", int(row), int(col), format(value, ".17g")])

        if actual_shape != shape:
            raise AssertionError(f"{name}: C++ changed matrix shape")
        np.testing.assert_array_equal(actual_rows, expected_rows, err_msg=f"{name}/rows")
        np.testing.assert_array_equal(actual_cols, expected_cols, err_msg=f"{name}/cols")
        np.testing.assert_allclose(actual_values, expected_values, rtol=1e-12, atol=0.0,
                                   equal_nan=False, err_msg=f"{name}/values")
        results.append((name, len(entries), len(actual_values), "PASS"))
        print(f"PASS: {name}")
    with report.open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["case", "input_entries", "canonical_nnz", "status"])
        writer.writerows(results)
    print(f"CoordinateMatrix SciPy validation: PASS ({len(results)} cases)")
    print(f"Results: {args.output_dir.resolve()}")


if __name__ == "__main__":
    main()
