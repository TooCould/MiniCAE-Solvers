"""Compare actual C++ Vector outputs against NumPy; preserve input/output files."""

import argparse
import csv
import io
from pathlib import Path
import subprocess
import sys

import numpy as np


def cases():
    yield "empty", [], [], 2.0
    yield "hand_calculated", [1.0, -2.0, 3.0], [4.0, 5.0, -6.0], 2.0
    yield "unit", [1.0, 0.0], [0.0, 1.0], -1.0
    yield "zero_alpha", [1.5, -2.5], [3.0, 4.0], 0.0
    yield "large_norm", [3e200, -4e200], [0.0, 0.0], 0.5
    yield "small_norm", [3e-200, -4e-200], [0.0, 0.0], 0.5
    rng = np.random.default_rng(20261106)
    for size in (1, 3, 8, 64, 257):
        yield f"random_{size}", rng.normal(size=size), rng.normal(size=size), -1.25


def read_output(output):
    reader = csv.DictReader(io.StringIO(output))
    if reader.fieldnames != ["operation", "index", "value"]:
        raise ValueError("Unexpected C++ CSV header")
    values = {}
    for row in reader:
        operation = row["operation"]
        entries = values.setdefault(operation, [])
        if int(row["index"]) != len(entries):
            raise ValueError(f"Unexpected index for {operation}")
        entries.append(float(row["value"]))
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path, help="Path to export_vector executable")
    parser.add_argument("--output-dir", type=Path, default=Path("out/validation/vector"))
    args = parser.parse_args()
    executable = args.exe.resolve(strict=True)
    print(f"Python: {sys.version.split()[0]}; NumPy: {np.__version__}")
    print(f"C++ executable: {executable}")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    # Remove the previous success report so a failed rerun cannot leave a stale PASS.
    report_path = args.output_dir / "summary.csv"
    if report_path.exists():
        report_path.unlink()
    results = []
    for name, x_values, y_values, alpha in cases():
        x = np.asarray(x_values, dtype=np.float64)
        y = np.asarray(y_values, dtype=np.float64)
        input_text = (
            f"{x.size} {alpha:.17g}\n"
            + " ".join(format(value, ".17g") for value in x) + "\n"
            + " ".join(format(value, ".17g") for value in y) + "\n"
        )
        (args.output_dir / f"{name}_input.txt").write_text(input_text, encoding="utf-8")
        run = subprocess.run(
            [str(executable)], input=input_text, capture_output=True,
            text=True, encoding="utf-8", timeout=10, check=True,
        )
        (args.output_dir / f"{name}_cpp.csv").write_text(run.stdout, encoding="utf-8")
        actual = read_output(run.stdout)
        expected = {
            "size": np.array([x.size], dtype=np.float64),
            "add": x + y,
            "subtract": x - y,
            "scale_left": alpha * x,
            "scale_right": x * alpha,
            "negate": -x,
            "axpy": alpha * x + y,
            "dot": np.array([np.dot(x, y)]),
            # hypot.reduce avoids intermediate squaring overflow/underflow in norm.
            "norm2": np.array([np.hypot.reduce(x, initial=0.0)]),
        }
        if set(actual) - set(expected):
            raise ValueError(f"{name}: unexpected C++ operations")
        with (args.output_dir / f"{name}_numpy.csv").open("w", newline="", encoding="utf-8") as file:
            writer = csv.writer(file)
            writer.writerow(["operation", "index", "value"])
            for operation, reference in expected.items():
                # Empty vector operations emit no rows; every scalar must be present.
                observed = np.asarray(actual.get(operation, []), dtype=np.float64)
                if observed.shape != reference.shape:
                    raise AssertionError(f"{name}/{operation}: result size mismatch")
                # No absolute floor: zero must not pass as a tiny nonzero result.
                np.testing.assert_allclose(
                    observed, reference, rtol=1e-12, atol=0.0, equal_nan=False,
                    err_msg=f"{name}/{operation}",
                )
                for index, value in enumerate(reference):
                    writer.writerow([operation, index, format(value, ".17g")])
        results.append((name, x.size, "PASS"))
        print(f"PASS: {name}")
    with report_path.open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["case", "size", "status"])
        writer.writerows(results)
    print(f"Vector NumPy validation: PASS ({len(results)} cases)")
    print(f"Results: {args.output_dir.resolve()}")


if __name__ == "__main__":
    main()
