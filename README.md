# Experimental OpenMS 4 Core SDK

Independent C++23 scientific SDK extracted from OpenMS develop at
`ca32296038839459d8c9b075b759e285913d6294`, with experimental ABI version **4.0.0**.
This repository builds `OpenMS::Core`, `OpenMS::OpenSwathAlgo`, their scientific
class tests, and optional development test support. CLI tools, desktop applications,
and pyOpenMS consume the installed SDK from separate repositories.

## Build and test Core

Use CMake 3.24 or newer, a C++23 compiler, and installed native dependencies.
`core-debug` uses shared Core/Boost/Arrow libraries and enables scientific class
tests and TestSupport. Opentims, Thermo RAW, WNet, TDL, ONNX, and HDF5 support are
disabled in this first portable profile; their CMake switches remain available.
Qt and downstream application builds are absent.

From this repository's root, with dependencies on the normal discovery path:

```bash
cmake --preset core-debug
cmake --build --preset core-debug --parallel 2
ctest --preset core-debug --parallel 2
```

For custom native dependency locations, add
`-DCMAKE_PREFIX_PATH="/absolute/dependency/prefix;/another/prefix"` to configuration.
On Apple Silicon macOS, select Homebrew curl explicitly so an older system-local
`libcurl.framework` cannot override the intended native dependency:

```bash
cmake --preset core-debug -U 'CURL_*' \
  -DCMAKE_PREFIX_PATH="/opt/homebrew;/opt/homebrew/opt/libomp" \
  -DCURL_ROOT=/opt/homebrew/opt/curl -DCMAKE_FIND_FRAMEWORK=LAST
```

The `-U` argument removes cached curl discovery results before the explicit root is
applied. Match the compiler, architecture and Debug configuration when building SDK
consumers, and pass the same curl/framework settings to their configuration when
needed. Use `core-release` for Release builds. Both presets accept an explicit
CMake toolchain, or `-DOPENMS_USE_VCPKG=ON` after initializing the vcpkg submodule;
platform and architecture are selected by the toolchain or generator.
External Percolator comparisons use `-DPERCOLATOR_BINARY_FOR_TEST=/absolute/path/to/percolator`
or PATH discovery; unavailable subprocess checks are reported explicitly. The
in-process scientific tests remain in Core.

Git builds observe the full commit and staged, unstaged and non-ignored untracked
changes. Add `-DOPENMS_REQUIRE_CLEAN_SOURCE=ON` for a publishable clean build.
Reconfigure after source identity changes. Source archives must explicitly provide
`-DOPENMS_SOURCE_REVISION=<40-character-commit> -DOPENMS_SOURCE_DIRTY=OFF` (or `ON`).
An invalid `OPENMS_DATA_PATH` throws `Exception::FileNotFound`; library callers may
correct the override and retry. Desktop styles and integration metadata are owned
by the separate desktop package.

The [compiler warning audit](COMPILER_WARNING_AUDIT.md) records the Linux build,
focused Clang checks, fixes and remaining vendor diagnostics.

## Install the SDK and optional TestSupport

A default `cmake --install` includes every enabled component, including TestSupport.
To exercise a complete SDK without TestSupport, select components into a fresh prefix:

```bash
OPENMS_CORE_PREFIX="$PWD/build/core-sdk"
for component in library OpenMS_headers OpenSwathAlgo_headers thirdparty_headers cmake share; do
  cmake --install build/core-debug --config Debug --prefix "$OPENMS_CORE_PREFIX" --component "$component"
done
```

Consumers then use `find_package(OpenMS 4.0.0 EXACT CONFIG REQUIRED)` and link
`OpenMS::Core`. Its package configuration records the exact Core source revision,
public dependency versions and enabled features. `VersionInfo::getSourceRevision()`,
`isSourceDirty()` and `getBuildInfo()` expose the loaded library identity;
`OpenMS_BUILD_INFO_FILE` locates the matching JSON, including compiler/runtime ABI. Core runtime data is versioned
under `share/OpenMS/4.0.0`; `OpenMS_DATA_DIR` exposes its location.

Install the optional development component after checking the SDK without it:

```bash
cmake --install build/core-debug --config Debug --prefix "$OPENMS_CORE_PREFIX" --component TestSupport
```

It provides `OpenMS::TestFramework`, the registration source and class-test fixtures,
selected with `find_package(OpenMS 4.0.0 EXACT CONFIG REQUIRED COMPONENTS TestSupport)`.
The `examples` install component is separate from runtime data and test support.

## Verify installed consumers

The [installed-SDK acceptance project](tests/installed_sdk_acceptance/README.md)
builds small independent consumers, tests public Eigen/Arrow APIs and mzML/Parquet
round trips, checks optional test support, and repeats after SDK relocation. Use a
clean, committed Core checkout so the expected revision identifies the built source:

```bash
python3 tests/installed_sdk_acceptance/run_acceptance.py \
  --sdk-prefix "$OPENMS_CORE_PREFIX" \
  --work-dir "$PWD/build/installed-acceptance" \
  --expected-revision "$(git rev-parse HEAD)" \
  --configuration Debug --jobs 2
```

Add `--dependency-prefix /absolute/dependency/prefix` when needed. For the SDK without
TestSupport, run it before adding that component, use a different empty work directory,
and pass `--cmake-argument=-DOPENMS_ACCEPTANCE_TEST_SUPPORT=OFF`.

Current native build, class-test and installed-consumer results are recorded in the
[superproject's Core native validation report](https://github.com/okohlbacher/OpenMS4-tests/blob/codex/package-split/docs/core-native-validation.md).
A successful build on one profile does not validate disabled native integrations or
other platforms. See [CORE_IMPLEMENTATION_NOTES.md](CORE_IMPLEMENTATION_NOTES.md)
for ownership, exports and validation boundaries.

## CI and SDK packages

GitHub Actions builds and runs the scientific tests on Linux x64/ARM64, macOS
x64/ARM64 and Windows x64 (MSVC). Each job uses the `core-release` profile and
pinned conda-forge dependencies. Windows Release matches the native dependencies'
MSVC runtime; use matching Debug dependencies for Windows development builds.
Optional native readers and inference integrations remain opt-in and are not
covered by this portable CI profile.

Every successful job installs the SDK, archives it, extracts the archive, and
builds independent consumers against both the extracted and relocated SDK. The
checks exercise public APIs, mzML/Parquet round trips, runtime data, TestSupport
and rejection of an incorrect Core revision. Logs record test results and timings.

Tags beginning with `core-v` publish experimental GitHub release assets only after
all platform jobs succeed. These are SDK archives, with headers, libraries,
runtime data, CMake package files and TestSupport. Their dependency manifest
records the exact conda packages used. Dependencies remain separately installed;
the archives do not bundle third-party libraries or platform runtimes. Consumers
must use compatible compiler/runtime settings and the matching dependency versions.

## Tool backend extraction

NuXL search algorithms, ProSEAlgorithm, FLASH search/deconvolution orchestration,
and adapter/workflow/comparison helpers now have independent product providers.
Core retains all reusable format readers/writers, including NuXLReport, MQ writers,
FLASHDeconv file writers and their shared record/scoring types. NA chemistry and
Comet modification parameter representation remain SDK APIs. The separate ProSE
and FLASH developer packages provide their moved C++ headers and shared libraries;
Python builds explicitly select those providers to retain their existing APIs.

This is an experimental API/ABI change: use a newly built SDK and rebuild consumers
with its exact source pin. Core tests remain independent of product libraries;
algorithm integration tests move to their owner. See the parent repository's
`docs/tool-backend-refactoring.md` and execution report for migration and validation.

## Upstream project

OpenMS is distributed under the BSD 3-Clause license. See [OpenMS](https://github.com/OpenMS/OpenMS) for the wider suite, scientific documentation and community links.
