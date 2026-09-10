# Experimental core SDK extraction

This package contains the scientific core and OpenSwathAlgo from upstream commit
`ca32296038839459d8c9b075b759e285913d6294`, with an experimental ABI version of
**4.0.0**. It is a source-complete extraction candidate, not a binary-validated SDK.

## Implemented ownership

- Root and `src/CMakeLists.txt` configure only scientific libraries, optional test
  support, and core/OpenSwathAlgo class tests. No product, Python, GUI, or documentation
  subdirectory is added. Existing platform/dependency detection remains available.
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
allowing in-tree scientific tests when `ENABLE_CLASS_TESTING=ON`.

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

## Validation performed without building OpenMS

`python3 -m unittest discover -s tests/sdk_contract -v` passed **10 tests**:
source/header and class-test registration closure; no core-to-CLI includes; local
fixture closure; mock installed-SDK discovery with preserved consumer options;
optional/missing TestSupport; rejection of wrong core/Arrow versions; and rejection
of an incompatible Arrow target flavour. The mocks also emulate dependency configs
changing CMake's `PACKAGE_PREFIX_DIR`, ensuring SDK data remains relocatable.

These tests run CMake script mode and a tiny `LANGUAGES NONE` mock consumer. They do
not configure or compile OpenMS or discover its native dependencies. New C++ files
passed the repository's clang-format check.

Future CTest cases `CoreDataPath_explicit_override` and
`CoreDataPath_invalid_override` were added, but **not run** because they require a
compiled core. Existing CTD/CWL round-trip tests cover serializer compatibility.

## Binary acceptance still required

1. Configure/build core and its class tests on supported platforms; run the tests.
2. Install runtime/header/CMake/data components, relocate the SDK, and remove access
   to the original source/build directories before compiling external consumers.
3. Compile consumers using Eigen and Arrow, and prove the matching Arrow dependency
   closure is loaded once in downstream Python processes.
4. Test library-only and optional TestSupport installations independently; compile
   downstream FuzzyDiff and class tests against the installed development component.
5. Run the fresh-process data override tests plus library-relative relocation tests
   with executables/Python outside the SDK prefix.
6. Validate compiler, standard library, build configuration, feature, and dependency
   identities against the superproject's artifact manifest before publishing an SDK.

Full project configuration, compilation, and binary tests remain intentionally
unperformed under the repository's explicit no-build constraint.

## Compiler-free data metadata

The SDK also installs `OpenMSDataConfig.cmake` under `lib/cmake/OpenMSData`.
`find_package(OpenMSData 4.0.0 EXACT CONFIG REQUIRED COMPONENTS TestSupport)` exposes
core version/revision, runtime/test-data directories, and feature metadata without
loading native dependencies or enabling a compiler language. Requiring TestSupport
fails if the class-fixture directory is absent. This supports fixture-only and
suite acceptance harnesses; compiled consumers continue to use `OpenMS` targets.
The metadata relocation/native-independence regression check passed with OpenMP
recorded as enabled, and checks missing/present optional fixture components.
