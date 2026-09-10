# OpenMS 4 Core SDK agent notes

This experimental repository owns the scientific C++ SDK and its class tests.
CLI, TOPP, desktop, FLASH products and Python bindings live in separate repositories
and consume an installed, pinned Core SDK. See README.md and CORE_IMPLEMENTATION_NOTES.md.

## Constraints

- Build only when the user explicitly authorizes it; coordinate resource-intensive
  native builds with the task owner. Use Debug for development.
- Never modify `src/openms/extern/`, `src/openms/thirdparty/`, contrib, or other
  third-party dependencies. Never commit secrets, credentials, or `.env` files.
- Run relevant tests after code changes; distinguish source/configuration checks
  from native compilation and scientific test results.
- Keep all unrelated user work. Commit with OpenMS tags such as `[BUILD,FIX,TEST]`.
- Do not add `using namespace` or `using std::...` in headers.

## Build and acceptance

Use CMake 3.24+, C++23 and the `core-debug` preset (`core-release` for delivery). The preset enables shared SDK
libraries, scientific class tests and TestSupport; optional Opentims, Thermo RAW,
HDF5, TDL, ONNX and WNet integrations remain selectable but are disabled in it.
There are no GUI, TOPP, Python, documentation or suite-installer build switches here.

```
cmake --preset core-debug
cmake --build --preset core-debug --parallel 2
ctest --preset core-debug --parallel 2
python3 -m unittest discover -s tests/sdk_contract -v
```

CI uses Release with conda-forge dependencies on five native platform/architecture
pairs. Windows uses the Visual Studio generator and matching Release CRT; do not
link those dependencies into an ordinary MSVC Debug build.

On macOS use shared Boost (`BOOST_USE_STATIC=OFF`), shared Arrow
(`ARROW_USE_STATIC=OFF`) and the curl/framework flags in README.md. Do not edit
native dependencies to work around host installation problems. Native optional
reader tests may need external fixtures; report which cases actually ran.

Install the library/header/data/CMake components first, then accept an ordinary
consumer and rejection of missing TestSupport. Install TestSupport separately and
run `tests/installed_sdk_acceptance` again from a fresh build directory, including
relocation. A default install includes TestSupport when enabled. It owns test
fixtures, registration source, TestFileValidation.h and the test framework; it does
not install FuzzyDiff (a downstream TOPP executable).

Git builds record the full source revision and non-ignored working-tree changes,
including untracked inputs. `OPENMS_REQUIRE_CLEAN_SOURCE=ON` rejects dirty builds;
source archives must assert both `OPENMS_SOURCE_REVISION` and
`OPENMS_SOURCE_DIRTY=ON/OFF`. Reconfigure after source identity changes.

## Source and tests

- Core headers: `src/openms/include/OpenMS`; implementations: `src/openms/source`.
- OpenSWATH algorithms: `src/openswathalgo`; standard-library test framework:
  `src/testframework`.
- Class tests: `src/tests/class_tests/openms/source`; fixtures:
  `src/tests/class_tests/openms/data`; OpenSWATH tests have their own directory.
- Register classes in the corresponding `sources.cmake` and tests in
  `executables.cmake`. Core tests must not include CLI/GUI headers or sibling data.
- Runtime probes: `tests/runtime`; independent installed consumer:
  `tests/installed_sdk_acceptance`; configuration tests: `tests/sdk_contract`.
- Runtime data belongs under `share/OpenMS`; desktop owns its stylesheet and
  desktop integration metadata. Keep scientific examples used by class tests.

Use existing ClassTest macros, `NEW_TMP_FILE`, local fixture paths and meaningful
observable assertions. A negative executable test must verify its diagnostic and
exit status when a generic nonzero result would hide an unrelated failure.
`OPENMS_DATA_PATH` is authoritative; invalid data discovery throws FileNotFound.
Applications own their error-reporting boundary, and callers can fix the path and retry.

## Code conventions

Use 2-space C++ and 4-space Python indentation, braces, Unix newlines, existing
`.clang-format`, PascalCase types and lowerCamelCase methods. Prefer standard
library ownership and existing OpenMS abstractions over new wrappers. Export
public non-template APIs with `OPENMS_DLLAPI`. Document public interfaces and
exceptions with Doxygen, including parameter directions. Preserve existing author
attribution; record AI assistance accurately without inventing contributors.

C++ API changes require coordination with the separate Python bindings package and
updates to tests, README/implementation notes and CHANGELOG. Runtime build identity
must agree byte-for-byte with the installed OpenMSBuildInfo.json. Never claim a
source commit alone establishes binary compatibility or validates optional features.
