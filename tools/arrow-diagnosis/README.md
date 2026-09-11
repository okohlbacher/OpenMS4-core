# Windows Parquet diagnosis

This branch contains isolated diagnostic programs; it is not a qualified Core SDK release.
The production package branch remains `codex/package-split`.

Core commit `c6adde13814ff2ab6d2e7d7b8a49af724bd4b73a` passed all 701 Windows
scientific tests and all eight original installed-SDK probes. Its relocated
`sdk_arrow` process exited with `0xc0000409`; the other seven probes passed.
The cause remains unestablished. No production Arrow setting was changed.

The standalone program writes two doubles, reads them, counts rows, removes the
file and checks the values. Comparisons cover default settings, disabled read-ahead,
explicit pool shutdown and the system allocator. All comparisons passed:

| Run | Scope | Successful process launches per comparison |
| --- | --- | ---: |
| [34566041641](https://github.com/okohlbacher/OpenMS4-core/actions/runs/34566041641) | Executable with stage output | 200 |
| [34566286526](https://github.com/okohlbacher/OpenMS4-core/actions/runs/34566286526) | Quiet executable | 1,000 |
| [34566606826](https://github.com/okohlbacher/OpenMS4-core/actions/runs/34566606826) | Quiet executable reading through a separate DLL | 1,000 |

These comparisons do not exercise OpenMS and do not prove the SDK failure fixed.
The manual workflow now accepts a native Core CI run ID, verifies/downloads that
Windows SDK, and exercises the original quiet SDK probe for 1,000 consecutive
CTest runs at each original/relocated prefix. The first failure fails acceptance;
this is not retry-until-success. Diagnostic runs cannot qualify a release.
