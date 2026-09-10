# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
#
# --------------------------------------------------------------------------
# $Maintainer: Stephan Aiche $
# $Authors: Stephan Aiche $
# --------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project("OpenMS_class_tests_openms")

#------------------------------------------------------------------------------
# Configure test file to get the TEST_DATA_PATH into the tests
set(CF_OPENMS_TEST_DATA_PATH "${PROJECT_SOURCE_DIR}/data/")
set (CONFIGURED_TEST_CONFIG_H "${PROJECT_BINARY_DIR}/include/OpenMS/test_config.h")
configure_file(${PROJECT_SOURCE_DIR}/include/OpenMS/test_config.h.in ${CONFIGURED_TEST_CONFIG_H})

#------------------------------------------------------------------------------
# get the test executables
include(executables.cmake)

#------------------------------------------------------------------------------
# Include directories for tests
set(OPENMS_CLASS_TESTS_INTERNAL_INCLUDE_DIRECTORIES "${PROJECT_BINARY_DIR}/include/;${PROJECT_SOURCE_DIR}/include/")
# add OpenMS directories
set(OPENMS_CLASS_TESTS_EXTERNAL_INCLUDE_DIRECTORIES "${OpenMS_INCLUDE_DIRECTORIES};${Boost_INCLUDE_DIRS}")
include_directories(${OPENMS_CLASS_TESTS_INTERNAL_INCLUDE_DIRECTORIES})
include_directories(SYSTEM ${OPENMS_CLASS_TESTS_EXTERNAL_INCLUDE_DIRECTORIES})

#------------------------------------------------------------------------------
# disable optimization for tests for gcc like compilers
if (CMAKE_COMPILER_IS_INTELCXX OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
  set(_TMP_CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  set(CMAKE_CXX_FLAGS_RELEASE "-O0")
endif()

#------------------------------------------------------------------------------
# Add the actual tests
# An OBJECT target keeps the registration object in every executable while
# compiling it only once. A static archive could discard its initializer.
add_library(OpenMSClassTestSupport OBJECT source/OpenMSTestSupport.cpp)
target_link_libraries(OpenMSClassTestSupport PRIVATE OpenMSTestFramework ${OpenMS_LIBRARIES})
openms_add_executable_compiler_flags(OpenMSClassTestSupport)
foreach(_class_test ${TEST_executables})
  # OpenMSTestSupport.cpp registers libOpenMS behavior (unique-ID seeding, exception
  # naming) with the std-only test framework. Removing it makes ID-bearing
  # reference-file comparisons nondeterministic -- see the file's header comment.
  add_executable(${_class_test} source/${_class_test}.cpp $<TARGET_OBJECTS:OpenMSClassTestSupport>)
  target_link_libraries(${_class_test} OpenMSTestFramework ${OpenMS_LIBRARIES})
  openms_add_executable_compiler_flags(${_class_test})
  add_test(${_class_test} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${_class_test})
  # only add OPENMP flags to gcc linker (except Mac OS X, due to compiler bug
  # see https://sourceforge.net/apps/trac/open-ms/ticket/280 for details)
  if (OPENMP_FOUND AND NOT MSVC AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_target_properties(${_class_test} PROPERTIES LINK_FLAGS ${OpenMP_CXX_FLAGS})
  endif()
endforeach(_class_test)

#------------------------------------------------------------------------------
# some tests need special treatment

# StopWatch_test needs to be run alone to avoid timing errors
set_tests_properties(StopWatch_test PROPERTIES RUN_SERIAL 1)

# PipEchoAlgorithm_realistic_test synthesises ~10k features across 3 runs and
# drives the LIBSVM-based MBR FDR path over many rounds. It is slow (seconds to
# minutes) and skips silently unless OPENMS_RUN_SLOW_TESTS is set, so it does
# not bloat the default CI run. Labelled "slow" so it can be excluded/selected
# explicitly (e.g. ctest -LE slow) and given a generous timeout for when it runs.
set_tests_properties(PipEchoAlgorithm_realistic_test PROPERTIES LABELS "slow" TIMEOUT 1800)

# Percolator_subprocess_parity_test runs side-by-side comparisons against the
# external `percolator` binary. The PIN-stamp section always runs; optional
# subprocess sections report their unavailable dependency explicitly. Set
# PERCOLATOR_BINARY_FOR_TEST to pin the external executable, or discover it
# on PATH without assuming any sibling source repository.
find_program(PERCOLATOR_BINARY_FOR_TEST NAMES percolator
  DOC "Optional external Percolator executable used by subprocess parity tests")
set_tests_properties(Percolator_subprocess_parity_test PROPERTIES
  LABELS "core;optional-integration")

if (PERCOLATOR_BINARY_FOR_TEST)
  set_tests_properties(Percolator_subprocess_parity_test PROPERTIES
    ENVIRONMENT "PERCOLATOR_BINARY=${PERCOLATOR_BINARY_FOR_TEST}")
  message(STATUS "Percolator_subprocess_parity_test: enabled (found ${PERCOLATOR_BINARY_FOR_TEST})")
else()
  message(STATUS "Percolator_subprocess_parity_test: PIN-stamp checks enabled; subprocess sections SKIPPED (set PERCOLATOR_BINARY_FOR_TEST or put percolator on PATH)")
endif()

#------------------------------------------------------------------------------
# Extra dependencies needed directly by individual tests. Boost regex is public
# on OpenMS and already propagates to every class test.
target_link_libraries(Base64_test ZLIB::ZLIB)
target_link_libraries(Libzip_test libzip::zip)
# Xerces is a PRIVATE dependency of libOpenMS. These tests exercise the internal
# Xerces InputSource / BinInputStream adapters directly, so they must link Xerces
# themselves.
target_link_libraries(ZipInputStream_test XercesC::XercesC)
target_link_libraries(GzipInputStream_test XercesC::XercesC)
target_link_libraries(Bzip2InputStream_test XercesC::XercesC)
target_link_libraries(CompressedInputSource_test XercesC::XercesC)
target_link_libraries(ZipRandomAccessFile_test libzip::zip)
target_link_libraries(LPWrapper_test ${LPTARGET})
target_link_libraries(SpectraSTSimilarityScore_test Eigen3::Eigen)
target_link_libraries(BinnedSpectrum_test Eigen3::Eigen)
target_link_libraries(BilinearInterpolation_test Eigen3::Eigen)
target_link_libraries(MatrixEigen_test Eigen3::Eigen)
if(TARGET MRMScoring_test)
  target_link_libraries(MRMScoring_test Eigen3::Eigen)
endif()


if (WITH_HDF5)
  target_link_libraries(HDF5_test HDF5::HDF5)
endif()

target_link_libraries(Arrow_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(MSExperimentArrowExport_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(ConsensusMapArrowExport_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(ProteinGroupArrowExport_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(QPXFile_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
# Builds its invalid inputs with arrow::FloatBuilder / ArrayData::Make rather than through the
# exporters, so it needs Arrow itself, not just what OpenMS re-exports. On Linux/macOS the symbols
# happen to be reachable through libOpenMS; on Windows they are dllimport and the link fails.
target_link_libraries(QPXValueValidation_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(ProteinIdentificationArrowIO_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(FeatureMapArrowIO_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(ConsensusMapArrowIO_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(PSMArrowIO_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(XICParquetFile_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(XIMParquetFile_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(Libzip_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)
target_link_libraries(ZipRandomAccessFile_test
  ${OPENMS_ARROW_TARGET}
  ${OPENMS_PARQUET_TARGET}
)

if (TARGET TransitionParquetFile_test)
  target_link_libraries(TransitionParquetFile_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
endif()
if (TARGET OpenSwathOSWParquetReader_test)
  target_link_libraries(OpenSwathOSWParquetReader_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
endif()
if (TARGET OpenSwathOSWParquetWriter_test)
  target_link_libraries(OpenSwathOSWParquetWriter_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
endif()
if (TARGET OpenSwathOSWParquetRoundTrip_test)
  target_link_libraries(OpenSwathOSWParquetRoundTrip_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
endif()
if (TARGET OpenSwathExport_test)
  target_link_libraries(OpenSwathExport_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
endif()
if (TARGET OpenSwathPercolatorScoring_test)
  target_link_libraries(OpenSwathPercolatorScoring_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
  #Test calls sqlite3 directly, so linkage required.
  if(OPENMS_USE_VCPKG AND TARGET unofficial::sqlite3::sqlite3)
    target_link_libraries(OpenSwathPercolatorScoring_test unofficial::sqlite3::sqlite3)
  elseif(TARGET SQLite::SQLite3)
    target_link_libraries(OpenSwathPercolatorScoring_test SQLite::SQLite3)
  elseif(TARGET SQLiteCpp)
    target_link_libraries(OpenSwathPercolatorScoring_test SQLiteCpp ${OPENMS_SQLITECPP_EXTRA_LIBS})
  elseif(TARGET sqlite3)
    target_link_libraries(OpenSwathPercolatorScoring_test sqlite3)
  endif()
endif()
if (TARGET ArrowSchemaRegistry_test)
  target_link_libraries(ArrowSchemaRegistry_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
endif()
if (TARGET OpenSwathResultsExporter_test)
  target_link_libraries(OpenSwathResultsExporter_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
endif()
if (TARGET OpenSwathParquetExporter_test)
  target_link_libraries(OpenSwathParquetExporter_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
endif()
if (TARGET ArrowIOHelpers_test)
  target_link_libraries(ArrowIOHelpers_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
endif()
if (TARGET ParquetFile_test)
  target_link_libraries(ParquetFile_test
    ${OPENMS_ARROW_TARGET}
    ${OPENMS_PARQUET_TARGET}
  )
endif()

if (TARGET BrukerTimsFile_test AND WITH_OPENTIMS)
  target_link_libraries(BrukerTimsFile_test opentims::opentims_cpp)
  if(OPENMS_USE_VCPKG AND TARGET unofficial::sqlite3::sqlite3)
    target_link_libraries(BrukerTimsFile_test unofficial::sqlite3::sqlite3)
  elseif(TARGET SQLite::SQLite3)
    target_link_libraries(BrukerTimsFile_test SQLite::SQLite3)
  elseif(TARGET SQLiteCpp)
    target_link_libraries(BrukerTimsFile_test SQLiteCpp ${OPENMS_SQLITECPP_EXTRA_LIBS})
  elseif(TARGET sqlite3)
    target_link_libraries(BrukerTimsFile_test sqlite3)
  endif()
endif()

if (TARGET BrukerTimsFile_test AND OPENTIMS_DDA_TEST_DATA)
  target_compile_definitions(BrukerTimsFile_test PRIVATE
    OPENTIMS_DDA_TEST_DATA="${OPENTIMS_DDA_TEST_DATA}"
    OPENTIMS_DIA_TEST_DATA="${OPENTIMS_DIA_TEST_DATA}"
    OPENTIMS_TEST_FASTA="${OPENTIMS_TEST_FASTA}")
  # This test loads real Bruker .d files (DDA + DIA) including frame aggregation,
  # a reader metadata assertions, and three-way (greedy vs. hill) centroiding
  # comparisons (which add 4 extra full-fixture loads), so it needs a generous timeout.
  set_tests_properties(BrukerTimsFile_test PROPERTIES TIMEOUT 3600)
endif()

if (TARGET ThermoRawFile_test AND WITH_THERMO_RAW AND THERMO_RAW_TEST_DATA)
  target_compile_definitions(ThermoRawFile_test PRIVATE
    THERMO_RAW_TEST_DATA="${THERMO_RAW_TEST_DATA}")
endif()

# Optional Thermo FAIMS-DIA integration fixture. It can be supplied explicitly via
# THERMO_FAIMS_DIA_TEST_DATA or downloaded by ENABLE_THERMO_FAIMS_DIA_TESTS.
if (TARGET ThermoRawFile_test AND WITH_THERMO_RAW AND THERMO_FAIMS_DIA_TEST_DATA)
  # Normalize manually supplied native paths (especially Windows paths with
  # backslashes) before embedding the value as a C/C++ string literal.
  file(TO_CMAKE_PATH "${THERMO_FAIMS_DIA_TEST_DATA}" _thermo_faims_dia_test_data)
  target_compile_definitions(ThermoRawFile_test PRIVATE
    THERMO_FAIMS_DIA_TEST_DATA="${_thermo_faims_dia_test_data}")
endif()

if (TARGET ThermoRawFile_test AND WITH_THERMO_RAW AND
    (THERMO_RAW_TEST_DATA OR THERMO_FAIMS_DIA_TEST_DATA))
  set_tests_properties(ThermoRawFile_test PROPERTIES TIMEOUT 1200)

  # Copy the managed bridge runtime (.dll + .runtimeconfig.json) next to the
  # test binary. The bridge looks first in the executable's own directory; on
  # Windows the test exe is in src/tests/class_tests/bin (not next to
  # libOpenMS), so the OpenMS-target POST_BUILD copy in src/openms/CMakeLists
  # is in a different directory and would not be found at test runtime.
  if (COMMAND openms_thermo_bridge_copy_runtime_files)
    openms_thermo_bridge_copy_runtime_files(TARGET ThermoRawFile_test)
  endif()
endif()

#------------------------------------------------------------------------------
# restore old compiler flags
if (CMAKE_COMPILER_IS_INTELCXX OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
  set(CMAKE_CXX_FLAGS_RELEASE "${_TMP_CMAKE_CXX_FLAGS_RELEASE}")
endif()

#------------------------------------------------------------------------------
# add filenames to Visual Studio solution tree
set(sources_VS)
foreach(i ${TEST_executables})
  list(APPEND sources_VS "${i}.cpp")
endforeach(i)
source_group("" FILES ${sources_VS})

if(WITH_ONNX)
  # Class tests run from the build-tree test directory and expect models below data/.
  # Reuse the models downloaded by src/openms/CMakeLists.txt instead of downloading
  # them again. No .onnx.data sidecars are needed because the exported models are
  # embedded-weight ONNX files.
  set(TEST_DATA_DIR "${CMAKE_BINARY_DIR}/src/tests/class_tests/openms/data")
  file(MAKE_DIRECTORY "${TEST_DATA_DIR}")

  foreach(MODEL IN LISTS OPENMS_PEPTDEEP_MODELS)
    set(SOURCE_MODEL "${OPENMS_PEPTDEEP_MODEL_DIR}/${MODEL}")
    set(TEST_MODEL "${TEST_DATA_DIR}/${MODEL}")

    if(NOT EXISTS "${SOURCE_MODEL}")
      message(FATAL_ERROR
        "Expected PeptDeep model was not found: ${SOURCE_MODEL}. "
        "The OpenMS ONNX model download step should have created this file.")
    endif()

    configure_file("${SOURCE_MODEL}" "${TEST_MODEL}" COPYONLY)
  endforeach()
endif()

# Runtime data resolution is cached; each policy case requires a fresh process.
add_executable(CoreDataPath_probe source/CoreDataPath_probe.cpp)
target_link_libraries(CoreDataPath_probe PRIVATE OpenMS::Core)
set(_data_override "${CMAKE_CURRENT_BINARY_DIR}/data-override")
file(MAKE_DIRECTORY "${_data_override}/CHEMISTRY")
file(COPY "${OPENMS_HOST_DIRECTORY}/share/OpenMS/CHEMISTRY/unimod.xml"
  DESTINATION "${_data_override}/CHEMISTRY")
add_test(NAME CoreDataPath_explicit_override
  COMMAND CoreDataPath_probe "${_data_override}")
set_tests_properties(CoreDataPath_explicit_override PROPERTIES
  ENVIRONMENT "OPENMS_DATA_PATH=${_data_override}")
add_test(NAME CoreDataPath_invalid_override
  COMMAND ${CMAKE_COMMAND}
    "-DPROBE_EXECUTABLE=$<TARGET_FILE:CoreDataPath_probe>"
    "-DPROBE_ARGUMENTS=${_data_override}"
    "-DINVALID_OVERRIDE=${CMAKE_CURRENT_BINARY_DIR}/missing-data-override"
    -P "${OPENMS_HOST_DIRECTORY}/tests/installed_sdk_acceptance/ExpectInvalidDataOverride.cmake")

add_test(NAME CoreDataPath_exception_retry
  COMMAND CoreDataPath_probe "${_data_override}" --retry)
set_tests_properties(CoreDataPath_exception_retry PROPERTIES
  ENVIRONMENT "OPENMS_DATA_PATH=${CMAKE_CURRENT_BINARY_DIR}/missing-data-override")

# The same probes run against the build and the installed SDK.
find_package(Threads REQUIRED)
add_executable(CoreIdentity_probe "${OPENMS_HOST_DIRECTORY}/tests/installed_sdk_acceptance/identity.cpp")
target_link_libraries(CoreIdentity_probe PRIVATE OpenMS::Core Threads::Threads)
add_test(NAME CoreIdentity_concurrent
  COMMAND CoreIdentity_probe "${OPENMS_HOST_BINARY_DIRECTORY}/identity/$<CONFIG>/OpenMSBuildInfo.json"
    "${OPENMS_SOURCE_REVISION}" "$<BOOL:${OPENMS_SOURCE_DIRTY}>")
add_executable(CoreExecutablePath_probe "${OPENMS_HOST_DIRECTORY}/tests/runtime/CoreExecutablePath_probe.cpp")
target_link_libraries(CoreExecutablePath_probe PRIVATE OpenMS::Core)
add_test(NAME CoreExecutablePath_unicode_and_long
  COMMAND "${CMAKE_COMMAND}" "-DPROBE_EXECUTABLE=$<TARGET_FILE:CoreExecutablePath_probe>"
    "-DWORK_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}"
    -P "${OPENMS_HOST_DIRECTORY}/tests/runtime/CheckExecutablePath.cmake")

add_executable(ProcessWait_test "${OPENMS_HOST_DIRECTORY}/tests/runtime/ProcessWait_test.cpp")
target_link_libraries(ProcessWait_test PRIVATE OpenMS::Core Threads::Threads)
add_test(NAME ProcessWait_test COMMAND ProcessWait_test)
set_tests_properties(ProcessWait_test PROPERTIES TIMEOUT 15)
