#!/usr/bin/env python3
"""Build small installed-SDK consumers, repeat after copying the SDK, and run CTest."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import time


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-prefix", type=Path, required=True)
    parser.add_argument("--work-dir", type=Path, required=True,
                        help="New or empty directory for consumers, relocated SDK, and results")
    parser.add_argument("--dependency-prefix", type=Path, action="append", default=[])
    parser.add_argument("--expected-revision", default="")
    parser.add_argument("--configuration", default="Debug")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--generator", default="Ninja")
    parser.add_argument("--cmake-argument", action="append", default=[],
                        help="Additional configure argument; use --cmake-argument=-DNAME=VALUE")
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    sdk = args.sdk_prefix.resolve(strict=True)
    work = args.work_dir.resolve()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if work.exists() and any(work.iterdir()):
        parser.error("--work-dir must be empty; existing results will not be overwritten")
    if sdk == work or sdk in work.parents or work in sdk.parents:
        parser.error("SDK and work directories must not overlap")
    work.mkdir(parents=True, exist_ok=True)
    results = []

    def run(command: list[str], *, expect_failure: bool = False) -> None:
        started = time.monotonic()
        completed = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, cwd=work)
        index = len(results) + 1
        log = work / f"{index:02d}.log"
        log.write_text(completed.stdout)
        passed = completed.returncode != 0 if expect_failure else completed.returncode == 0
        results.append({"command": command, "returncode": completed.returncode,
                        "expected_failure": expect_failure, "passed": passed,
                        "elapsed_seconds": round(time.monotonic() - started, 3),
                        "log": str(log)})
        (work / "results.json").write_text(json.dumps(results, indent=2) + "\n")
        print(completed.stdout, end="", flush=True)
        if not passed:
            raise SystemExit(f"Acceptance command did not satisfy its expectation; see {log}")

    def configure(prefix: Path, name: str, *, wrong_revision: bool = False) -> Path:
        configs = list(prefix.rglob("OpenMSConfig.cmake"))
        if len(configs) != 1:
            raise SystemExit(f"Expected one installed OpenMSConfig.cmake under {prefix}; found {configs}")
        build = work / name
        prefixes = [str(prefix), *(str(p.resolve()) for p in args.dependency_prefix)]
        command = ["cmake", "-S", str(source), "-B", str(build), "-G", args.generator,
                   f"-DCMAKE_BUILD_TYPE={args.configuration}",
                   f"-DCMAKE_PREFIX_PATH={';'.join(prefixes)}",
                   f"-DOpenMS_DIR={configs[0].parent}",
                   f"-DOPENMS_EXPECTED_PREFIX={prefix}", *args.cmake_argument]
        if wrong_revision:
            command.append("-DOPENMS_EXPECTED_SOURCE_REVISION=0000000000000000000000000000000000000000")
        elif args.expected_revision:
            command.append(f"-DOPENMS_EXPECTED_SOURCE_REVISION={args.expected_revision}")
        run(command, expect_failure=wrong_revision)
        if wrong_revision and "Core SDK source mismatch:" not in Path(results[-1]["log"]).read_text():
            raise SystemExit("Wrong-revision test failed for an unrelated reason; inspect its log")
        return build

    for prefix, name in [(sdk, "original-consumer"), (work / "relocated-sdk", "relocated-consumer")]:
        if prefix != sdk:
            # Preserve relative library symlinks. The original SDK remains in place,
            # and the runtime probe rejects accidentally loading from that location.
            shutil.copytree(sdk, prefix, symlinks=True)
        build = configure(prefix, name)
        run(["cmake", "--build", str(build), "--config", args.configuration,
             "--parallel", str(args.jobs)])
        run(["ctest", "--test-dir", str(build), "-C", args.configuration,
             "--output-on-failure", "--parallel", str(args.jobs)])
    configure(sdk, "wrong-revision-consumer", wrong_revision=True)
    print(f"Installed SDK acceptance passed. Results: {work / 'results.json'}")


if __name__ == "__main__":
    main()
