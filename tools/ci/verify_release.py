#!/usr/bin/env python3
"""Require one checksum-verified SDK per platform, built from the tagged commit."""

import hashlib
from pathlib import Path
import sys
import tarfile

PLATFORMS = ("linux-x64", "linux-arm64", "macos-x64", "macos-arm64", "windows-x64")


def verify(directory: Path, revision: str) -> None:
    archives = [directory / f"sdk-{platform}" /
                f"OpenMS4-core-{platform}-Release-{revision[:12]}.tar.gz" for platform in PLATFORMS]
    expected = set(archives) | {p.with_suffix(".gz.sha256") for p in archives}
    if {p for p in directory.rglob("*") if p.is_file()} != expected:
        raise ValueError("Release requires exactly five platform archives and their five checksum files")
    for archive in archives:
        with archive.open("rb") as stream:
            digest = hashlib.file_digest(stream, "sha256").hexdigest()
        if archive.with_suffix(".gz.sha256").read_text().strip() != f"{digest}  {archive.name}":
            raise ValueError(f"Checksum mismatch: {archive}")
        with tarfile.open(archive) as package:
            identity = package.extractfile(archive.name.removesuffix(".tar.gz") + "/source-revision.txt")
            if identity is None or identity.read().decode().strip() != revision:
                raise ValueError(f"Source revision mismatch: {archive}")
    print(f"Verified all five SDK archives for {revision}")


if __name__ == "__main__":
    verify(Path(sys.argv[1]), sys.argv[2])
