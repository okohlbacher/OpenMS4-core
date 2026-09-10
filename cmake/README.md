# Core SDK build helpers

The root build configures the scientific SDK and class tests. Consumers use its
installed CMake package; TOPP, desktop, Python and suite installers have separate
repositories. `Modules/` contains dependency discovery shared with installed SDKs.

`ENABLE_UNITYBUILD=ON` uses CMake's native unity batches for the SDK libraries,
including `SKIP_UNITY_BUILD_INCLUSION` and per-source compile-property exclusions.
The default builds each source separately.

Old third-party Windows installer files remain unchanged under `Windows/` solely
because vendored dependencies are protected from modification. No Core build or
installation rule references them.
