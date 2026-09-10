#!/usr/bin/env python3
"""Build the portable Core SDK and test the exact archive offered for deployment."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import time


def package_sdk(sdk: Path, dist: Path, deployed: Path, name: str) -> Path:
    """Archive an installed SDK, preserve library symlinks, and extract it for testing."""
    dist.mkdir(parents=True, exist_ok=True)
    archive = dist / f"{name}.tar.gz"
    with tarfile.open(archive, "w:gz") as output:
        output.add(sdk, arcname=name)
    with archive.open("rb") as stream:
        checksum = hashlib.file_digest(stream, "sha256").hexdigest()
    archive.with_suffix(archive.suffix + ".sha256").write_text(
        f"{checksum}  {archive.name}\n", encoding="utf-8")
    with tarfile.open(archive) as source:
        source.extractall(deployed, filter="data")
    return deployed / name


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", required=True,
                        choices=["linux-x64", "linux-arm64", "macos-x64", "macos-arm64", "windows-x64"])
    parser.add_argument("--work-dir", required=True, type=Path)
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[2]
    work = args.work_dir.resolve()
    if args.jobs < 1 or (work.exists() and any(work.iterdir())):
        parser.error("--jobs must be positive and --work-dir must be empty")
    if source == work or source in work.parents or work in source.parents:
        parser.error("--work-dir must be outside the source checkout")
    results = work / "results"
    results.mkdir(parents=True)
    build, sdk = work / "build", work / "sdk"
    dependencies = Path(os.environ["CONDA_PREFIX"]).resolve()
    windows = sys.platform == "win32"
    solver = "GLPK" if windows else "COIN"
    dependency_prefix = dependencies / "Library" if windows else dependencies
    env = os.environ.copy()
    env.update(OMP_NUM_THREADS="1", OPENMS_RUN_SLOW_TESTS="1", PYTHONUTF8="1")
    # Only third-party dependencies enter loader search paths. Core must resolve
    # from the build tree or the particular extracted/relocated SDK under test.
    if sys.platform == "linux":
        env["LD_LIBRARY_PATH"] = str(dependencies / "lib")
    elif sys.platform == "darwin":
        env["DYLD_FALLBACK_LIBRARY_PATH"] = str(dependencies / "lib")
    commands = []

    def run(name: str, command: list[str]) -> str:
        started = time.monotonic()
        print(f"\n--- {name} ---", flush=True)
        log = results / f"{name}.log"
        with log.open("w", encoding="utf-8") as stream:
            process = subprocess.Popen(command, cwd=source, env=env, text=True,
                                       encoding="utf-8", errors="replace", stdout=subprocess.PIPE,
                                       stderr=subprocess.STDOUT)
            for line in process.stdout:
                stream.write(line)
                print(line, end="", flush=True)
            code = process.wait()
        commands.append({"name": name, "command": command, "returncode": code,
                         "elapsed_seconds": round(time.monotonic() - started, 3)})
        (results / "commands.json").write_text(json.dumps(commands, indent=2) + "\n", encoding="utf-8")
        if code:
            raise subprocess.CalledProcessError(code, command)
        return log.read_text(encoding="utf-8")

    revision = run("source-revision", ["git", "rev-parse", "HEAD"]).strip()
    run("ci-driver-tests", [sys.executable, "-m", "unittest", "discover", "-s", "tools/ci", "-v"])
    run("source-contract-tests", [sys.executable, "-m", "unittest", "discover", "-s", "tests/sdk_contract", "-v"])
    run("test-wrapper-tests", [sys.executable, "-m", "unittest", "discover", "-s", "tests/installed_sdk_acceptance", "-v"])
    explicit = run("dependencies", ["micromamba", "list", "--explicit"])
    inventory = run("dependency-inventory", ["micromamba", "list", "--json"])
    generator = "Visual Studio 17 2022" if windows else "Ninja"
    cmake_args = [f"-DCMAKE_PREFIX_PATH={dependency_prefix.as_posix()}",
                  f"-DCURL_ROOT={dependency_prefix.as_posix()}", "-DCMAKE_FIND_FRAMEWORK=LAST",
                  f"-DLP_SOLVER={solver}", "-DSTL_DEBUG=OFF"]
    if windows:
        cmake_args += ["-A", "x64", "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL"]
    elif sys.platform == "darwin":
        cmake_args += ["-DCMAKE_C_COMPILER=/usr/bin/clang", "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
                       f"-DOpenMP_ROOT={dependency_prefix.as_posix()}"]
    run("configure", ["cmake", "--preset", "core-release", "-B", str(build), "-G", generator,
                      f"-DCMAKE_INSTALL_PREFIX={sdk.as_posix()}", "-DOPENMS_REQUIRE_CLEAN_SOURCE=ON", *cmake_args])
    run("build", ["cmake", "--build", str(build), "--config", "Release", "--parallel", str(args.jobs)])
    run("scientific-tests", ["ctest", "--test-dir", str(build), "-C", "Release", "--output-on-failure",
                             "--no-tests=error", "--parallel", str(args.jobs),
                             "--output-junit", str(results / "scientific-tests.xml")])
    run("install", ["cmake", "--install", str(build), "--config", "Release"])
    (sdk / "dependencies.txt").write_text(explicit, encoding="utf-8")
    (sdk / "dependencies.json").write_text(inventory, encoding="utf-8")
    shutil.copyfile(source / "tools/ci/environment.yml", sdk / "environment.yml")
    (sdk / "source-revision.txt").write_text(revision + "\n", encoding="utf-8")
    deployed = package_sdk(sdk, work / "dist", work / "deployed",
                           f"OpenMS4-core-{args.platform}-Release-{revision[:12]}")
    # The original installation is unavailable when deployed consumers run.
    sdk.rename(work / "unpublished-sdk")
    acceptance_args = [f"--cmake-argument={arg}" for arg in cmake_args if arg.startswith("-D")]
    if windows:
        acceptance_args.append("--cmake-argument=-DCMAKE_GENERATOR_PLATFORM=x64")
    run("deployed-sdk-tests", [sys.executable, str(source / "tests/installed_sdk_acceptance/run_acceptance.py"),
                               "--sdk-prefix", str(deployed), "--work-dir", str(work / "acceptance"),
                               "--dependency-prefix", str(dependency_prefix), "--expected-revision", revision,
                               "--configuration", "Release", "--generator", generator,
                               "--jobs", str(args.jobs), *acceptance_args])
    if os.environ.get("GITHUB_STEP_SUMMARY"):
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as summary:
            summary.write(f"### {args.platform}: SDK built, all enabled tests passed, archive deployment verified\n\n")
            summary.write(f"Linear programming backend tested: {solver}.\n\n")
            summary.write("| Stage | Seconds |\n|---|---:|\n")
            for command in commands:
                summary.write(f"| {command['name']} | {command['elapsed_seconds']:.3f} |\n")


if __name__ == "__main__":
    main()
