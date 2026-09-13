#!/usr/bin/env python3
"""Point a disposable CI formula at the exact source archive under review."""

import argparse
import hashlib
from pathlib import Path
import re


def prepare_formula(formula: str, revision: str, url: str, checksum: str) -> str:
    if not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise ValueError("Expected a full Git source revision")
    if not re.fullmatch(r"[0-9a-f]{64}", checksum):
        raise ValueError("Expected an archive SHA-256")
    if not re.fullmatch(r"https://github\.com/[\w.-]+/[\w.-]+/archive/" + revision + r"\.tar\.gz", url):
        raise ValueError("Archive URL must name the same GitHub source revision")
    # Released bottles must never satisfy the candidate source build.
    formula = re.sub(r"(?ms)^  bottle do\n.*?^  end\n", "", formula)
    replacements = (
        (r'^  url ".*"$', f'  url "{url}"'),
        (r'^  version ".*"$', f'  version "4.0.0-ci.{revision[:12]}"'),
        (r'^  sha256 ".*"$', f'  sha256 "{checksum}"'),
        (r"-DOPENMS_SOURCE_REVISION=[0-9a-f]{40}", f"-DOPENMS_SOURCE_REVISION={revision}"),
    )
    for pattern, replacement in replacements:
        formula, count = re.subn(pattern, replacement, formula, flags=re.MULTILINE)
        if count != 1:
            raise ValueError(f"Expected exactly one formula field matching {pattern}")
    return formula


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--formula", type=Path, required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--url", required=True)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    checksum = hashlib.sha256(args.archive.read_bytes()).hexdigest()
    args.output.write_text(prepare_formula(args.formula.read_text(), args.revision, args.url, checksum))
