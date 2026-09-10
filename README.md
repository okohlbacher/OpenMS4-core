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
needed. The platform vcpkg presets remain available separately.
External Percolator comparisons use `-DPERCOLATOR_BINARY_FOR_TEST=/absolute/path/to/percolator`
or PATH discovery; unavailable subprocess checks are reported explicitly. The
in-process scientific tests remain in Core.

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
public dependency versions and enabled features. Core runtime data is versioned
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

## Historical upstream overview

The following overview is retained from the upstream OpenMS 3.6 source. Its suite,
GUI and Python descriptions refer to the wider OpenMS project; the Core SDK workflow
above applies to this repository.

---

OpenMS
=======

[![License (3-Clause BSD)](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg?logo=data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0idXRmLTgiPz4NCjwhLS0gR2VuZXJhdG9yOiBBZG9iZSBJbGx1c3RyYXRvciAyNC4xLjEsIFNWRyBFeHBvcnQgUGx1Zy1JbiAuIFNWRyBWZXJzaW9uOiA2LjAwIEJ1aWxkIDApICAtLT4NCjxzdmcgdmVyc2lvbj0iMS4xIiBpZD0iTGF5ZXJfMSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIiB4bWxuczp4bGluaz0iaHR0cDovL3d3dy53My5vcmcvMTk5OS94bGluayIgeD0iMHB4IiB5PSIwcHgiDQoJIHZpZXdCb3g9IjAgMCA1MTIgNTEyIiBzdHlsZT0iZW5hYmxlLWJhY2tncm91bmQ6bmV3IDAgMCA1MTIgNTEyOyIgeG1sOnNwYWNlPSJwcmVzZXJ2ZSI+DQo8Zz4NCgk8cGF0aCBkPSJNMCwyNjcuMkMyLjQsMTI2LjksMTAwLjYsMjcuMSwyMjAuOCwxMUMzNjQuMS04LjIsNDg0LjcsODkuMyw1MDcuOSwyMTguNmMyMiwxMjIuNy00NS40LDIzNy41LTE1Ni41LDI4Mi45DQoJCWMtOS42LDMuOS0xNC45LDEuOC0xOC42LTcuOWMtMTguNC00Ny44LTM2LjgtOTUuNy01NS4xLTE0My41Yy0zLjItOC40LTEtMTMuNiw3LjItMTcuNGMyNS0xMS40LDQwLjYtMzAuNCw0NC43LTU3LjYNCgkJYzMuMS0yMC4yLTIuMy00MC44LTE0LjktNTYuOWMtMTIuNi0xNi4xLTMxLjQtMjYuMi01MS44LTI4Yy00MC4zLTMuNS03NC4xLDI0LjUtODAsNjEuNmMtNS40LDM0LjEsMTEuNSw2NS44LDQzLjMsODAuMg0KCQljOS45LDQuNSwxMS45LDguOSw4LjEsMTljLTE4LjUsNDguMS0zNyw5Ni4zLTU1LjQsMTQ0LjVjLTIuNyw3LjEtOC42LDkuNi0xNiw2LjdjLTU0LjMtMjEtMTA0LjctNjMtMTM1LjEtMTIyLjkNCgkJQzIsMzI4LjYsMS43LDI4OC44LDAsMjY3LjJMMCwyNjcuMnogTTIxLjYsMjY1LjJDMjIsMjcyLDIyLjIsMjgwLDIyLjksMjg4YzYuNSw3NC4zLDUxLjIsMTQ4LjIsMTM1LjMsMTg5LjENCgkJYzMuMywxLjUsNC41LDAuOCw1LjgtMi40YzE1LjQtNDAuNCwzMC45LTgwLjgsNDYuNS0xMjEuMWMxLjMtMy40LDAuNi01LTIuNS02LjljLTMyLjYtMjAuNi00OC44LTUwLjEtNDcuMS04OC44DQoJCWMxLTIyLjMsOS42LTQxLjgsMjQuNi01OC4xYzMxLTMzLjgsNzkuNS00MS4xLDExOS4zLTE4LjJjMzIuOSwxOSw1MS4yLDU1LjcsNDYuNyw5My4zYy0zLjcsMzEuNi0xOS45LDU1LjctNDcuMiw3Mi4xDQoJCWMtMi44LDEuNy0zLjYsMy0yLjQsNi4yYzE1LjcsNDAuNSwzMS4zLDgxLDQ2LjcsMTIxLjVjMS4yLDMuMiwyLjUsMy45LDUuOCwyLjRjMzYuNy0xNy4xLDY3LjMtNDEuNiw5MS03NC4zDQoJCUM0ODEuMiwzNTMsNDk2LDI5Ny41LDQ4OSwyMzYuNUM0NzQuOCwxMTUuMSwzNjUuNywxNC43LDIyNS4xLDMyQzExNS42LDQ1LjQsMjMuNSwxMzcuOCwyMS42LDI2NS4yTDIxLjYsMjY1LjJ6Ii8+DQo8L2c+DQo8L3N2Zz4NCg==&style=flat-square)](http://opensource.org/licenses/BSD-3-Clause)
[![Project Stats](https://www.openhub.net/p/open-ms/widgets/project_thin_badge.gif)](https://www.openhub.net/p/open-ms)
[![Discord Shield](https://img.shields.io/discord/832282841836159006?style=flat-square&message=Discord&color=5865F2&logo=Discord&logoColor=FFFFFF&label=Discord)](https://discord.gg/4TAGhqJ7s5)
[![Gitter](https://img.shields.io/static/v1?style=flat-square&message=on%20Gitter&color=ED1965&logo=Gitter&logoColor=FFFFFF&label=Chat)](https://gitter.im/OpenMS/OpenMS?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)
[![Install with bioconda](https://img.shields.io/conda/v/bioconda/pyopenms?color=44A833&logo=Anaconda&style=flat-square&label=Install%20with%20bioconda)](http://bioconda.github.io/recipes/openms-meta/README.html)
[![Install with conda](https://img.shields.io/conda/v/openms/pyopenms?color=44A833&label=Install%20with%20conda%3A%3Aopenms&logo=Anaconda&style=flat-square)](https://anaconda.org/openms)
[![Documentation](https://img.shields.io/static/v1?style=flat-square&message=ReadTheDocs&color=2C4AA8&logo=ReadTheDocs&logoColor=FFFFFF&label=Documentation)](https://openms.readthedocs.io)
[![API docs](https://img.shields.io/static/v1?style=flat-square&message=Doxygen&color=2C4AA8&logo=ReadTheDocs&logoColor=FFFFFF&label=API%20docs)](https://abibuilder.cs.uni-tuebingen.de/archive/openms/Documentation/release/latest/html/index.html)
[![Gitpod Ready-to-Code](https://img.shields.io/badge/Gitpod-Ready--to--Code-blue?style=flat-square&logo=gitpod)](https://gitpod.io/#https://github.com/OpenMS/OpenMS)
[![Open in Dev Containers](https://img.shields.io/static/v1?label=Dev%20Containers&message=Open&color=blue&logo=data:image/svg%2bxml;base64,PHN2ZyB3aWR0aD0iMTAwIiBoZWlnaHQ9IjEwMCIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48bWFzayBpZD0iYSIgbWFzay10eXBlPSJhbHBoYSIgbWFza1VuaXRzPSJ1c2VyU3BhY2VPblVzZSIgeD0iMCIgeT0iMCIgd2lkdGg9IjEwMCIgaGVpZ2h0PSIxMDAiPjxwYXRoIGZpbGwtcnVsZT0iZXZlbm9kZCIgY2xpcC1ydWxlPSJldmVub2RkIiBkPSJNNzEgOTloNWwyMC0xMGMzLTEgNC0zIDQtNVYxNmMwLTItMS00LTQtNUw3NiAxYTYgNiAwIDAgMC03IDFMMjkgMzggMTIgMjVIN2wtNiA1djZsMTUgMTRMMSA2NHY2bDYgNWg1bDE3LTEzIDQwIDM2IDIgMVptNC03Mkw0NSA1MGwzMCAyM1YyN1oiIGZpbGw9IiNmZmYiLz48L21hc2s+PGcgbWFzaz0idXJsKCNhKSI+PHBhdGggZD0iTTk2IDExIDc2IDFjLTMtMS01LTEtNyAxTDEgNjRjLTIgMS0yIDQgMCA2bDYgNWg1bDgxLTYyYzMtMiA3IDAgNyA0di0xYzAtMi0xLTQtNC01WiIgZmlsbD0iIzAwNjVBOSIvPjxnIGZpbHRlcj0idXJsKCNiKSI+PHBhdGggZD0iTTk2IDg5IDc2IDk5Yy0zIDEtNSAxLTctMUwxIDM2Yy0yLTEtMi00IDAtNmw2LTVoNWw4MSA2MmMzIDIgNyAwIDctNHYxYzAgMi0xIDQtNCA1WiIgZmlsbD0iIzAwN0FDQyIvPjwvZz48ZyBmaWx0ZXI9InVybCgjYykiPjxwYXRoIGQ9Ik03NiA5OWMtMyAxLTUgMS03LTEgMiAyIDYgMSA2LTNWNWMwLTQtNC01LTYtMyAyLTIgNC0yIDctMWwyMCAxMGMzIDEgNCAzIDQgNXY2OGMwIDItMSA0LTQgNUw3NiA5OVoiIGZpbGw9IiMxRjlDRjAiLz48L2c+PHBhdGggZmlsbC1ydWxlPSJldmVub2RkIiBjbGlwLXJ1bGU9ImV2ZW5vZGQiIGQ9Ik03MSA5OWg1bDIwLTEwYzMtMSA0LTMgNC01VjE2YzAtMi0xLTQtNC01TDc2IDFhNiA2IDAgMCAwLTcgMUwyOSAzOCAxMiAyNUg3bC02IDVjLTIgMi0yIDUgMCA2bDE1IDE0TDEgNjRjLTIgMS0yIDQgMCA2bDYgNWg1bDE3LTEzIDQwIDM2IDIgMVptNC03Mkw0NSA1MGwzMCAyM1YyN1oiIGZpbGw9InVybCgjZCkiIHN0eWxlPSJtaXgtYmxlbmQtbW9kZTpvdmVybGF5IiBvcGFjaXR5PSIuMyIvPjwvZz48ZGVmcz48ZmlsdGVyIGlkPSJiIiB4PSItOCIgeT0iMTYiIHdpZHRoPSIxMTYuNyIgaGVpZ2h0PSI5Mi4yIiBmaWx0ZXJVbml0cz0idXNlclNwYWNlT25Vc2UiIGNvbG9yLWludGVycG9sYXRpb24tZmlsdGVycz0ic1JHQiI+PGZlRmxvb2QgZmxvb2Qtb3BhY2l0eT0iMCIgcmVzdWx0PSJCYWNrZ3JvdW5kSW1hZ2VGaXgiLz48ZmVDb2xvck1hdHJpeCBpbj0iU291cmNlQWxwaGEiIHZhbHVlcz0iMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAgMTI3IDAiLz48ZmVPZmZzZXQvPjxmZUdhdXNzaWFuQmx1ciBzdGREZXZpYXRpb249IjQuMiIvPjxmZUNvbG9yTWF0cml4IHZhbHVlcz0iMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAgMC4yNSAwIi8+PGZlQmxlbmQgbW9kZT0ib3ZlcmxheSIgaW4yPSJCYWNrZ3JvdW5kSW1hZ2VGaXgiIHJlc3VsdD0iZWZmZWN0MV9kcm9wU2hhZG93Ii8+PGZlQmxlbmQgaW49IlNvdXJjZUdyYXBoaWMiIGluMj0iZWZmZWN0MV9kcm9wU2hhZG93IiByZXN1bHQ9InNoYXBlIi8+PC9maWx0ZXI+PGZpbHRlciBpZD0iYyIgeD0iNjAiIHk9Ii04IiB3aWR0aD0iNDcuOSIgaGVpZ2h0PSIxMTYuMiIgZmlsdGVyVW5pdHM9InVzZXJTcGFjZU9uVXNlIiBjb2xvci1pbnRlcnBvbGF0aW9uLWZpbHRlcnM9InNSR0IiPjxmZUZsb29kIGZsb29kLW9wYWNpdHk9IjAiIHJlc3VsdD0iQmFja2dyb3VuZEltYWdlRml4Ii8+PGZlQ29sb3JNYXRyaXggaW49IlNvdXJjZUFscGhhIiB2YWx1ZXM9IjAgMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDEyNyAwIi8+PGZlT2Zmc2V0Lz48ZmVHYXVzc2lhbkJsdXIgc3RkRGV2aWF0aW9uPSI0LjIiLz48ZmVDb2xvck1hdHJpeCB2YWx1ZXM9IjAgMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAgMCAwIDAuMjUgMCIvPjxmZUJsZW5kIG1vZGU9Im92ZXJsYXkiIGluMj0iQmFja2dyb3VuZEltYWdlRml4IiByZXN1bHQ9ImVmZmVjdDFfZHJvcFNoYWRvdyIvPjxmZUJsZW5kIGluPSJTb3VyY2VHcmFwaGljIiBpbjI9ImVmZmVjdDFfZHJvcFNoYWRvdyIgcmVzdWx0PSJzaGFwZSIvPjwvZmlsdGVyPjxsaW5lYXJHcmFkaWVudCBpZD0iZCIgeDE9IjQ5LjkiIHkxPSIuMyIgeDI9IjQ5LjkiIHkyPSI5OS43IiBncmFkaWVudFVuaXRzPSJ1c2VyU3BhY2VPblVzZSI+PHN0b3Agc3RvcC1jb2xvcj0iI2ZmZiIvPjxzdG9wIG9mZnNldD0iMSIgc3RvcC1jb2xvcj0iI2ZmZiIgc3RvcC1vcGFjaXR5PSIwIi8+PC9saW5lYXJHcmFkaWVudD48L2RlZnM+PC9zdmc+)](https://vscode.dev/redirect?url=vscode://ms-vscode-remote.remote-containers/cloneInVolume?url=https://github.com/microsoft/vscode-remote-try-java)

[OpenMS](http://www.openms.org/)
is an open-source software C++ library for LC-MS data management and
analyses. It offers an infrastructure for rapid development of mass
spectrometry-related software. OpenMS is free software available under the
three-clause BSD license and runs under Windows, macOS, and Linux.

It comes with a vast variety of pre-built and ready-to-use tools for proteomics
and metabolomics data analysis (TOPPTools) as well as powerful 1D, 2D and 3D
visualization (TOPPView).

OpenMS offers analyses for various quantitation protocols, including label-free
quantitation, SILAC, iTRAQ, TMT, SRM, SWATH, etc.

It provides built-in algorithms for de novo identification and database search,
as well as adapters to other state-of-the-art tools like Comet, etc.
It supports easy integration of OpenMS built tools into workflow
engines like nextflow, KNIME, Galaxy, and TOPPAS via the TOPPTools concept and
a unified parameter handling via a 'common tool description' (CTD) scheme.

With pyOpenMS, OpenMS offers Python bindings to a large part of the OpenMS API
to enable rapid algorithm development. OpenMS supports the Proteomics Standard
Initiative (PSI) formats for MS data. The main contributors of OpenMS are
currently the Eberhard-Karls-Universität in Tübingen, the Freie Universität
Berlin, and the ETH Zürich.

![Alt](https://repobeats.axiom.co/api/embed/f6f2225d0071dd56f77a66c997ea89c1396a035d.svg "Repobeats analytics image")

Table of Contents
--------
- [Features](#features)
- [Documentation](#documentation)
- [Building OpenMS](#building-openms)
- [Citation](#citation)

Features
--------
- Core C++ library under three-clause BSD licence using modern C++23
- Python bindings to the C++ API through pyOpenMS
- Major community file formats supported (mzML, mzXML, mzIdentXML, pepXML, mzTab, etc.)
- Over 150+ individual analysis tools (TOPP Tools), covering most MS and LC-MS data processing and mining tasks
- Powerful 1D, 2D and 3D visualization tools (TOPPView)
- Support for most MS identification and quantification workflows (targeted, DIA, label-free, isobaric and stable isotope labelled)
- Support for all major platforms (Windows [10, 11], macOS and Linux)

Documentation
-------------

Users and developers should start by reading the [OpenMS documentation](https://openms.readthedocs.io/en/latest) on ReadTheDocs (RTD).
The OpenMS API reference and advanced developer doxygen documentation can be browsed [here](https://abibuilder.cs.uni-tuebingen.de/archive/openms/Documentation/release/latest/html/index.html).

The OpenMS RTD documentation aims at being an entry point for users and developers alike. It is trying to be mostly version-independent and therefore
only consists of one main branch. We may introduce tags for older releases in the future.

The OpenMS API reference has several endpoints:

1. [`nightly`](https://abibuilder.cs.uni-tuebingen.de/archive/openms/Documentation/nightly/html/index.html): OpenMS API reference and advanced developer documentation of nightly releases.
2. [`release/latest`](https://abibuilder.cs.uni-tuebingen.de/archive/openms/Documentation/release/latest/html/index.html) : OpenMS API reference and advanced developer documentation of latest stable release.
3. [`release/${version}`](https://abibuilder.cs.uni-tuebingen.de/archive/openms/Documentation/release/latest/html/index.html) : OpenMS API reference and advanced developer documentation of an older version.

Documentation for the Python bindings pyOpenMS can be found on the [pyOpenMS online documentation](https://pyopenms.readthedocs.io).

Building OpenMS
--------

For developers who want to build OpenMS from source:

- [Build on Linux](https://abibuilder.cs.uni-tuebingen.de/archive/openms/Documentation/release/latest/html/install_linux.html) - Build instructions for Linux.
- [Build on Windows](https://abibuilder.cs.uni-tuebingen.de/archive/openms/Documentation/release/latest/html/install_win.html) - Build instructions for Windows.
- [Build on macOS](https://abibuilder.cs.uni-tuebingen.de/archive/openms/Documentation/release/latest/html/install_mac.html) - Build instructions for macOS.

For more detailed instructions, see the [Developer Tutorial](https://abibuilder.cs.uni-tuebingen.de/archive/openms/Documentation/release/latest/html/developer_tutorial.html).

Citation
--------
Please cite:

Pfeuffer, J., Bielow, C., Wein, S. et al. OpenMS 3 enables reproducible analysis of large-scale mass spectrometry data, Nat Methods 21, 365–367 (2024). https://doi.org/10.1038/s41592-024-02197-7

The file [AUTHORS](AUTHORS) contains a list of all authors who worked on OpenMS.
