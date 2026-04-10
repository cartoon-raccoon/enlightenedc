"""
Parses each HolyC .HC file in /test/input, checks stdout outputs for correctness, and outputs corresponding debug IR representations to /test/output.
"""

import subprocess
from pathlib import Path
import sys

input_dir = Path("./input")
output_dir = Path("./output")
expected_dir = Path("./expected")

output_dir.mkdir(exist_ok=True)

ECC_PATH = "../build/ecc"
EXECUTABLE = "./a.out"


def run_test(file):
    name = file.stem

    ast_outfile = output_dir / f"{name}_ast.txt"

    print(f"\n=== Testing {name} ===")

    with open(ast_outfile, "w") as ast_file:
        result = subprocess.run(
            [ECC_PATH, str(file)],
            stdout=subprocess.DEVNULL,
            stderr=ast_file
        )

    if result.returncode != 0:
        print(f"[FAIL] Compilation failed for {name}")
        return False

    run = subprocess.run(
        [EXECUTABLE],
        capture_output=True,
        text=True
    )

    actual = run.stdout.strip()

    expected_file = expected_dir / f"{name}.txt"
    if not expected_file.exists():
        print(f"[WARN] No expected output for {name}")
        print(actual)
        return True

    expected = expected_file.read_text().strip()

    if actual == expected:
        print(f"[PASS] {name}")
        return True
    else:
        print(f"[FAIL] {name}")
        print("---- EXPECTED ----")
        print(expected)
        print("---- ACTUAL ----")
        print(actual)

        if run.stderr:
            print("---- RUNTIME STDERR ----")
            print(run.stderr)

        return False


def main():
    failures = 0

    for file in sorted(input_dir.glob("*.HC")):
        if not run_test(file):
            failures += 1

    print("\n------------------------")
    if failures == 0:
        print("ALL TESTS PASSED")
    else:
        print(f"{failures} TEST(S) FAILED")
        sys.exit(1)


if __name__ == "__main__":
    main()