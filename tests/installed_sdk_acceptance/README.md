# Installed Core SDK acceptance

This is an external CMake project. It consumes only an installed OpenMS 4.0.0 SDK through `find_package(OpenMS EXACT CONFIG)` and imported targets. It does not add the Core source tree, tools, GUI, or Python packages to its build.

The seven runtime checks cover:

- An mzML round trip, spectrum operations, and chemistry database access through the public API.
- The SDK's exported Eigen include path and a zero-copy OpenMS matrix view.
- Arrow's public headers and symbols, plus an actual Parquet round trip, through `OpenMS::Core` alone.
- Data resolution relative to the loaded library from an executable outside the SDK.
- A valid explicit `OPENMS_DATA_PATH` override.
- Rejection of an invalid override rather than fallback to another installation.
- Installed TestSupport headers, fixture data, and deterministic unique-ID registration.

The runner performs the same build and tests after copying the SDK to another prefix, while leaving the original prefix present. The data probe checks the resolved data directory **and the actual loaded core library path**, so accidentally using the original installation does not pass. Dependencies such as Arrow and Boost remain at their installed locations: this tests relocation of the OpenMS SDK, not a self-contained redistributable containing every third-party library. It then verifies that a consumer requesting the wrong source revision is rejected.

The negative data-override check requires exit code 1 and the exact OpenMS diagnostic naming the rejected path. Its shared `ExpectInvalidDataOverride.cmake` wrapper invokes the probe directly and rejects signals, runtime-loader errors, unrelated failures, and unexpected success. Both the Core class suite and this consumer project use that wrapper; they do not treat an arbitrary failing process as a successful negative test. The wrapper's script-level regression checks run without Core or a native build:

```bash
python3 -m unittest discover -s tests/installed_sdk_acceptance -p 'test_*.py' -v
```

After building and installing Core with its TestSupport component, run:

```bash
python3 tests/installed_sdk_acceptance/run_acceptance.py \
  --sdk-prefix /absolute/path/to/core-install \
  --work-dir /absolute/path/to/new-acceptance-directory \
  --dependency-prefix /absolute/path/to/dependencies \
  --expected-revision FULL_CORE_COMMIT_SHA \
  --configuration Debug \
  --jobs 2
```

Repeat `--dependency-prefix` for multiple dependency installations. On macOS, pass Homebrew's `libomp` prefix if the SDK uses OpenMP and its package discovery requires it. Any required toolchain configuration can be added with `--cmake-argument=-DCMAKE_TOOLCHAIN_FILE=/absolute/path/to/toolchain.cmake`. Match the compiler, architecture, C++ runtime, and build configuration used by Core. This runner builds only the five small consumer executables, and never builds Core or downloads dependencies.

For a library-only SDK without TestSupport, configure this project directly with `-DOPENMS_ACCEPTANCE_TEST_SUPPORT=OFF`; that runs the six other checks. A direct build requires `-DOPENMS_EXPECTED_PREFIX=/absolute/path/to/sdk` and an explicit `OpenMS_DIR` or `CMAKE_PREFIX_PATH`. Pass `-DOPENMS_EXPECTED_SOURCE_REVISION=FULL_CORE_COMMIT_SHA` to enforce provenance. Separate build directories are required for different SDK prefixes.

Successful source inspection or CMake configuration alone is not acceptance. The runner's `results.json` and numbered logs record each real configure, compilation, and runtime test result. Only report a native acceptance pass after this runner succeeds.
