"""
Parses each EnlightenedC .ec file in /test/input and outputs corresponding ast text representations to /test/output.
"""

import subprocess
from pathlib import Path

input_dir = Path("./input")
output_dir = Path("./output")
output_dir.mkdir(exist_ok=True)

for file in input_dir.glob("*.HC"):
    outfile = output_dir / f"{file.stem}_ast.txt"
    print(f"Parsing {file} -> {outfile}")
    result = subprocess.run(["../build/ecc", str(file)], stdout=open(outfile, "w"))
    if result.returncode != 0:
        print(f"Parsing failed for {file}")
