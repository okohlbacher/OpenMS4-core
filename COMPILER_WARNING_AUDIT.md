# Core compiler warning audit — 2026-09-10

The complete Linux/GCC Debug build reduced first-party compiler diagnostics from
**94 to zero**. Eight diagnostics remain in untouched vendored headers. Focused
AppleClang compilation of all 48 changed or affected translation units also has
zero first-party warnings; three Percolator diagnostics remain there.

## Scope and reproducibility

The baseline is Core commit `6bfc0e4711105f4eda2fea86812a83af7c7e791f`.
The final uncommitted source/test patch has SHA-256
`c538838be7847c859f120155eb098db83a85320a99bd03136886e957ba0029b0`.
Documentation-only changes are excluded from that patch. The Linux build and
installed SDK correctly report a dirty source tree, rather than claiming to be
binaries of the unchanged baseline commit.

Linux used GCC 14.4, C++23, the `core-debug` preset, shared libraries and
Boost/Arrow, COIN, OpenMP, and `STL_DEBUG=OFF`. The build ran on `dax` under
`/scratch/kohlbach/openms4-5d1e239-20260910/core-warning-audit`, with 320 build
jobs and 128 CTest jobs. Compiler caches and unity builds were disabled.
`OPENMS_RUN_SLOW_TESTS=1` exercised the full realistic PipEcho test.
The pre-build node load was 2.69 across 384 logical CPUs.

AppleClang 21.0.0 on macOS arm64 compiled the 48 translation units using their
normal generated CMake commands and warning flags. The final ZIP test fixture
was then recompiled separately. This was focused object compilation, not a
complete linked macOS SDK validation. A full macOS baseline was stopped after
419 of 2,202 build steps because of contention with other workloads.

The existing warning flags, including `-Wall -Wextra` and the project's existing
exclusions, were preserved. No global warning suppression or dependency changes
were added. Optional native integrations disabled by `core-debug` remain outside
this audit, as do Release, Windows and other compiler versions.

## Corrections

- Reused `ParquetFile` error helpers for 26 Arrow append operations and 59 array
  finalizers, retaining the column name and original error instead of ignoring
  status or aborting.
- Kept the protein-description string alive during median-normalization regex
  matching, eliminating a dangling pointer.
- Removed the erroneous `noexcept` from `AA::fromIndex`, so its documented Debug
  precondition exception can be caught. Added valid-boundary and invalid-index
  checks. The helper is not exposed by the separate Python bindings.
- Reused the existing UTF-8 path conversion helper and replaced obsolete
  `zip_replace` calls with `zip_file_replace(..., 0)`, which is the documented
  equivalent. ZIP failures now copy their diagnostic before freeing the archive
  handle. See the [libzip compatibility documentation](https://libzip.org/documentation/zip_add/).
- Replaced the four deprecated Boost.Process timed waits with one private helper
  using nonblocking process status and a monotonic deadline. Successful and
  failed exit codes are retained; timeout cleanup remains owned by the callers.
- Corrected signed comparisons and aggregate initialization, removed unused
  internal code, and matched conditional helpers to the features using them.
  Four unused private fields remain explicitly marked to preserve SDK class
  layouts; the missing override annotation was added.
- Replaced tautological tests with actual mapping/content/exception checks,
  initialized test inputs, corrected pointer comparisons and boolean expressions,
  and removed unused test helpers.

## Verification

Exact commands, timings and warning locations are recorded in the companion
`COMPILER_WARNING_AUDIT.json`. Raw logs, patches and commands are preserved in the
workspace's `core-warning-execution` directory.

- Clean Linux baseline build: **52.596 s**, 700/700 baseline tests in **49.547 s**.
- Clean Linux build with all production fixes: **53.286 s**, zero first-party
  warnings. The first test run exposed an incorrect new fixture: libzip accepts
  empty ZIP entry names. The fixture now supplies invalid UTF-8, which exercises
  the intended libzip error path without corrupting subsequent test setup.
- Final Core suite: **701/701 passed**, **48.065 s** (CTest reports 48.02 s).
  Rebuilding the corrected ZIP fixture took **1.902 s** with no warnings.
- SDK contract tests: **17/17 passed**, **25.022 s**.
- Independent installed and relocated SDK consumers: passed, including rejection
  of an incorrect source pin; **19.262 s** for the acceptance driver.
- AppleClang: **48/48 object compilations**, **66.154 s**, zero first-party
  warnings; final ZIP test compilation **2.214 s**, zero warnings.
- Seven focused Arrow allocation/finalization failure probes passed. They use
  zero-budget Arrow memory pools and a failing finalizer, with no production test
  hook. Persistent class tests cover nullable values, booleans and empty output.
- The process-wait executable passed success, nonzero exit and timeout cases on
  Linux and macOS; an additional native probe confirmed child-process cleanup.

The macOS runtime probes used the already installed compatible Abseil library
at `/opt/homebrew/Cellar/abseil/20260107.1/lib` via `DYLD_LIBRARY_PATH`; the current
Homebrew default points to a different version. No installed dependency files
were changed. Two macOS configure warnings came from Arrow's fallback searches
for lz4 and Thrift; the native libraries were found successfully.

## Remaining vendor diagnostics

| Source | GCC diagnostic lines | Cause |
| --- | ---: | --- |
| `src/openms/extern/Quadtree/include/Vector2.h` | 1 | Constructor uses a template-id disallowed in C++20 |
| `src/openms/thirdparty/percolator/ScoreHolder.h` | 6 | Two constructors initialize members out of declaration order |
| `src/openms/thirdparty/percolator/SetHandler.h` | 1 | Ignored qualifier on a value return type |

These files were not modified, as required by `AGENTS.md`. GCC emits multiple
warning-labelled lines for each initialization-order issue; counts above are
compiler diagnostic lines, not distinct bugs. Clang emits three lines for the
same Percolator issues and did not warn about Quadtree in the focused scope.
