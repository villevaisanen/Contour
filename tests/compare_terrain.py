# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Ville Väisänen
"""Compile one driver against the baseline and current core; compare all outputs."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
BASELINE = "6abab03973f967398b8a4a0fee8a77b54f01a624"


def main():
    baseline = subprocess.check_output(
        ["git", "show", BASELINE + ":src/core.hpp"], cwd=ROOT
    )
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    with tempfile.TemporaryDirectory(prefix="contour-equivalence-") as directory:
        work = Path(directory)
        original = work / "original_core.hpp"
        original.write_bytes(baseline)
        results = []
        for name, header in [("original", original), ("fixed", ROOT / "src/core.hpp")]:
            binary = work / name
            subprocess.run(
                compiler
                + ["-std=c++11", "-O3", "-funsafe-math-optimizations", "-UNDEBUG",
                   '-DTERRAIN_HEADER="' + str(header) + '"',
                   str(ROOT / "tests/terrain_dump.cpp"), "-o", str(binary)],
                check=True,
            )
            results.append(subprocess.check_output([str(binary)]))
        if results[0] != results[1]:
            raise SystemExit("FAIL: original and current terrain/motion/CV results differ")
        print("PASS: bit-exact 2.1.2/current comparison: 768 terrains, four seeds, "
              "all 1-4 contour levels, 48 time/fractality pairs; geometry metadata, "
              "points, distances, motion and CV (" + str(len(results[0])) + " bytes)")


if __name__ == "__main__":
    main()
