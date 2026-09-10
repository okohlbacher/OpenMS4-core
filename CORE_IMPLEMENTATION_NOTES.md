# Experimental core SDK extraction

This package contains the scientific core and OpenSwathAlgo from upstream commit
`ca32296038839459d8c9b075b759e285913d6294`, with an experimental ABI version of
**4.0.0**. Native build and acceptance status, including the tested platform,
feature profile and source revision, is maintained in the
[superproject validation report](https://github.com/okohlbacher/OpenMS4-tests/blob/codex/package-split/docs/core-native-validation.md).

## Implemented ownership

- Root and `src/CMakeLists.txt` configure only scientific libraries, optional test
  support, and core/OpenSwathAlgo class tests. No product, Python, GUI, or documentation
  subdirectory is added. Core orchestration no longer discovers Qt or prepares
  suite installers/documentation. Existing scientific dependency detection remains available.
- `ConsoleUtils` stays in core at its historical `OpenMS/APPLICATIONS` include path.
  All other application classes belong to downstream packages.
- `OpenMS/DATASTRUCTURES/ParamTags.h` owns the six parameter metadata strings used
  by CTD/CWL serializers. No serializer includes TOPPBase. The CLI owns legacy aliases.
- Stale OpenSwathBase includes in CalibrationWorkflow and an unused TOPPBase include
  in Param_test were removed. TransformationModelDefaults retains its scientific
  checks; its MapAlignerBase compatibility comparison belongs in CLI tests.
- Four distinct fixtures formerly referenced under TOPP are copied to
  `src/tests/class_tests/openms/data/extracted_topp`; `CORE_EXTRACTION_FIXTURES.txt`
  records their original relative paths. The PercolatorAdapter parity test belongs
  to TOPP. Core subprocess parity remains optional on an external Percolator binary.
  `PERCOLATOR_BINARY_FOR_TEST` selects it explicitly; otherwise CMake searches PATH.
  No sibling `THIRDPARTY` tree is assumed, and unavailable subprocess checks report
  their missing dependency. The test retains its always-run PIN-stamp checks.
- Core class tests compile `OpenMSTestSupport.cpp` once as the
  `OpenMSClassTestSupport` OBJECT target and include that object in every executable.
  This preserves deterministic unique-ID/exception registration without repeatedly
  compiling the same source or risking removal of its initializer by archive linking.
- All 2,213 retained `src/openms/extern` files and 84 retained
  `src/openms/thirdparty` files were compared byte-for-byte with upstream and were
  unchanged. No contrib or third-party source was modified.

## Installed contract

Consumers use `find_package(OpenMS 4.0.0 EXACT CONFIG REQUIRED)` with an explicitly
selected installed SDK prefix. They link `OpenMS::Core`, `OpenMS::OpenSwathAlgo`, and
when needed `OpenMS::Arrow`. Historical `OpenMS` and `OpenSwathAlgo` targets are aliases.
No user package registry entry is registered.

Arrow and Parquet versions are exact; the target names record shared/static linkage.
Public Arrow/Parquet types, Boost regex, and Eigen headers receive their required
transitive targets. Missing Arrow target flavours fail package discovery. Native
binary compatibility additionally requires the artifact manifest maintained by the
superproject; version and source commit alone do not establish ABI compatibility.

Metadata includes `OpenMS_VERSION`, full `OpenMS_SOURCE_REVISION`,
`OpenMS_CXX_STANDARD`, `OpenMS_CXX_COMPILER_ID`, `OpenMS_CXX_COMPILER_VERSION`,
`OpenMS_BUILD_TYPE`, `OpenMS_SHARED_LIBS`, `OpenMS_ADDCXX_FLAGS`,
`OpenMS_TESTING_HOOKS`, and `OpenMS_WITH_*` feature settings. Consumer variables such
as `WITH_GUI`, `WITH_HDF5`, and `BUILD_TOPP_TOOLS` are left untouched.

A package Git checkout records its own full HEAD. Source archive builders must
provide its exact package revision through `OPENMS_SOURCE_REVISION`; the upstream
provenance commit is not incorrectly reused as the refactored package identity.

Test support is an optional separate **TestSupport** install component, selected by
`find_package(OpenMS 4.0.0 EXACT CONFIG REQUIRED COMPONENTS Core TestSupport)`. It
exports `OpenMS::TestFramework`, `OpenMS_TEST_SUPPORT_SOURCE`,
`OpenMS_TEST_SUPPORT_INCLUDE_DIR`, `OpenMS_TEST_CONFIG_TEMPLATE`, and
`OpenMS_TEST_DATA_DIR`. Its export file is separate from `OpenMSTargets.cmake`, so a
library-only installation does not require the test archive or fixture data.
`OPENMS_BUILD_TEST_SUPPORT=OFF` disables installing this component while still
allowing in-tree scientific tests when `ENABLE_CLASS_TESTING=ON`. When enabled,
a default installation includes it. The README lists the explicit components
needed for a library/header/data/CMake installation without TestSupport; adding
`--component TestSupport` later installs the optional development payload.

## Runtime data

The default data installation is `share/OpenMS/4.0.0`. Use `OpenMS_DATA_DIR` or
`OpenMS_SHARE_DIR` from the SDK configuration, rather than constructing paths.
Legacy uppercase directory variables remain aliases.

An explicit `OPENMS_DATA_PATH` is authoritative and an invalid override fails.
Otherwise the loaded core library determines the versioned data location before
executable-relative and compiled-in developer fallbacks. This enables Python and
executables installed in separate prefixes to use the SDK's data after relocation.
The value is cached on first use. Installed RPATH defaults are library-relative and
do not include the build directory. Product executable discovery remains in CLI.

## Portable build profile

`core-debug` configures shared Core/Boost/Arrow libraries in Debug mode, with core
and OpenSwathAlgo class tests plus TestSupport enabled. It uses installed system or
contrib dependencies and disables optional Opentims, Thermo RAW, WNet, TDL, ONNX and
HDF5 features. These integrations remain selectable through their existing switches;
this profile does not claim to cover them. The README provides configure, build,
CTest and selective-install commands. Native dependencies and compiler settings must
match when building an external consumer.

The removal of application headers exposes missing transitive includes: ParamCTDFile
now includes StringUtils directly. The version test uses the generated Core version
constants, and data-resolution diagnostics describe the explicit override policy.
Scientific test fixtures remain local; runtime examples are retained while tests
such as InternalCalibration still use them.

## Validation layers

`python3 -m unittest discover -s tests/sdk_contract -v` checks source/header and
class-test registration closure, Core-to-CLI boundaries, local fixtures, mock
installed-SDK discovery, optional TestSupport, exact version rejection, and Arrow
target compatibility. These checks exercise CMake scripts and small mock consumers;
they do not establish native compilation or numerical correctness.

Core CTest covers the scientific class tests and the fresh-process data-path cases
`CoreDataPath_explicit_override` and `CoreDataPath_invalid_override`. Optional external
fixtures, native readers and slow tests must be reported according to what actually
ran; they must not be included in an unconditional acceptance claim.

`tests/installed_sdk_acceptance` is a separate CMake project that consumes only
installed imported targets. It compiles public-API, Eigen, Arrow/Parquet and data-path
probes, plus a TestSupport consumer when selected. The runner repeats compilation
and runtime checks from a copied SDK prefix, verifies both the loaded library and
resolved data location, and rejects a mismatched Core source revision. It tests
OpenMS SDK relocation while native dependencies remain at their installed locations;
it does not produce a self-contained third-party bundle.

For component acceptance, first install the SDK without TestSupport, prove ordinary
consumer use and rejection of a required missing TestSupport component, then install
TestSupport and run the full consumer checks. Use separate acceptance build/work
directories so cached package paths cannot mix the two cases. Record native evidence
and remaining failures in the [superproject validation report](https://github.com/okohlbacher/OpenMS4-tests/blob/codex/package-split/docs/core-native-validation.md).

Before publishing binaries, record the exact source commit, compiler, architecture,
standard-library ABI, build configuration, features and dependency identities, then
hash the files that actually passed validation. Python wheels, downstream products,
disabled integrations and other platforms require their own acceptance runs.

## Compiler-free data metadata

The SDK also installs `OpenMSDataConfig.cmake` under `lib/cmake/OpenMSData`.
`find_package(OpenMSData 4.0.0 EXACT CONFIG REQUIRED COMPONENTS TestSupport)` exposes
core version/revision, runtime/test-data directories, and feature metadata without
loading native dependencies or enabling a compiler language. Requiring TestSupport
fails if the class-fixture directory is absent. This supports fixture-only and
suite acceptance harnesses; compiled consumers continue to use `OpenMS` targets.
Its mock metadata test checks relocation with OpenMP recorded as enabled and
missing/present optional fixture components, without discovering native libraries.
