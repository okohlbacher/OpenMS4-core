# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
#
# --------------------------------------------------------------------------
# $Maintainer: Julianus Pfeuffer $
# $Authors: Stephan Aiche, Julianus Pfeuffer $
# --------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

# The vcpkg toolchain must be selected before project() initializes compilers.
# Existing system/contrib dependency discovery remains the default.
option(OPENMS_USE_VCPKG "Use vcpkg manifest mode for dependency management." OFF)

if(OPENMS_USE_VCPKG AND NOT CMAKE_TOOLCHAIN_FILE)
  set(_openms_submodule_vcpkg_toolchain "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

  if(EXISTS "${_openms_submodule_vcpkg_toolchain}")
    set(CMAKE_TOOLCHAIN_FILE "${_openms_submodule_vcpkg_toolchain}" CACHE FILEPATH "CMake toolchain file")
  else()
    message(FATAL_ERROR
      "OPENMS_USE_VCPKG is ON, but no vcpkg toolchain was found. Initialize the "
      "vcpkg submodule or set CMAKE_TOOLCHAIN_FILE.")
  endif()

  unset(_openms_submodule_vcpkg_toolchain)
endif()

# Handle default build type
get_property(multiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT multiConfig AND NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build.")
endif()

project("OpenMSCoreSDK" LANGUAGES CXX)

#------------------------------------------------------------------------------
# General CMake definitions & helper
#------------------------------------------------------------------------------
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)
#------------------------------------------------------------------------------
## CMake sanity check: sometimes CMAKE_SIZEOF_VOID_P just vanishes when
## updating CMake.
if (NOT CMAKE_SIZEOF_VOID_P)
	message(FATAL_ERROR "'CMAKE_SIZEOF_VOID_P' is undefined. Thus you should"
                      " delete CMakeFiles (the directory) and the"
                      " CMakeCache.txt and rerun CMake again! This is some"
                      " weird CMake bug that seems to appear when updating the"
                      " CMake version.")
endif()

#------------------------------------------------------------------------------
# Enable testing with CTest
enable_testing()

#------------------------------------------------------------------------------
# entries meant to be configured using CMake cache
# - do NOT hardcode them here!
# - edit them within CMakeCache.txt using ccmake or cmake -DVAR=VALUE
#------------------------------------------------------------------------------
# CMAKE_PREFIX_PATH
# CMAKE_BUILD_TYPE
# STL_DEBUG
# ADDRESS_SANITIZER
# OPENMS_COVERAGE
#------------------------------------------------------------------------------
option(MT_ENABLE_OPENMP "Enable OpenMP support" ON)
option(BOOST_USE_STATIC "Use Boost static libraries." ON)
option(INSTALL_OPENMS_EXAMPLES "Install the OpenMS example data." ON)
option(WITH_HDF5 "Build HDF5 parts of OpenMS." OFF)
option(WITH_OPENTIMS "Build with Bruker TimsTOF .d file support via opentims" ON)
option(ENABLE_OPENTIMS_TESTS "Download Bruker test data for opentims integration tests" OFF)
set(_openms_with_thermo_raw_default ON)
# Thermo's RawFileReader does not ship native dependencies for Linux aarch64,
# so the .NET runtime can build the bridge but throws a managed exception when
# actually opening a RAW file. Default OFF on that platform.
# macOS arm64 is natively supported as of openms-thermo-bridge v0.2.0.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64$" AND UNIX AND NOT APPLE)
  set(_openms_with_thermo_raw_default OFF)
endif()
option(WITH_THERMO_RAW "Build with Thermo RAW file reading support via openms-thermo-bridge" ${_openms_with_thermo_raw_default})
option(ENABLE_THERMO_RAW_TESTS "Download Thermo RAW test data for integration tests" OFF)
option(ENABLE_THERMO_FAIMS_DIA_TESTS "Download Thermo FAIMS-DIA test data for integration tests" OFF)
option(ENABLE_TDL "Load dependency and compile against TDL (required for CWL file support)." OFF)
option(ENABLE_CLASS_TESTING "Build scientific core class tests." ON)
option(OPENMS_BUILD_TEST_SUPPORT "Build and install the optional TestSupport development component." ON)
option(ARROW_USE_STATIC "Use static Arrow libraries (experimental; shared is preferred)." OFF)
option(WITH_ONNX "Build ONNX inference support." OFF)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64$" AND UNIX AND NOT APPLE)
  if(WITH_THERMO_RAW)
    message(WARNING
      "Thermo RAW support is disabled by default on Linux aarch64 because "
      "Thermo RawFileReader native dependencies are not available there. You explicitly "
      "enabled WITH_THERMO_RAW=ON. Please vote on https://github.com/thermofisherlsms/RawFileReader/issues/21 "
      "and contact Thermo Fisher support by email if this support matters for your workflow.")
  else()
    message(STATUS
      "Thermo RAW support defaults to OFF on Linux aarch64. Please vote on "
      "https://github.com/thermofisherlsms/RawFileReader/issues/21 and contact Thermo Fisher "
      "support by email if native support matters for your workflow.")
  endif()
endif()

if(MSVC)
  option(MT_ENABLE_NESTED_OPENMP "Enable nested parallelism." OFF)
else()
  option(MT_ENABLE_NESTED_OPENMP "Enable nested parallelism." ON)
endif()

if (ENABLE_OPENTIMS_TESTS AND WITH_OPENTIMS)
  include(FetchContent)
  # Mirror of the timsrust_cpp_bridge "test-data-v1" release, hosted on the OpenMS
  # infra fileserver (archive.openms.de). NOTE: this host currently answers only on
  # plain HTTP (no HTTPS listener yet) and port 80 is reachable only from whitelisted
  # CI/build networks at the perimeter firewall. URL_HASH pins each file's
  # integrity so the weaker transport cannot corrupt a download silently.
  set(OPENTIMS_TESTDATA_URL "http://archive.openms.de/openms/testfiles")
  FetchContent_Declare(
    opentims_testdata_dda
    URL "${OPENTIMS_TESTDATA_URL}/DDA_HeLa_50ng_5_6min.d.zip"
    URL_HASH SHA256=659d691e48d621edcd1fecb8448de5731147c009bda42ba6e7e5ed53e46dc15c
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_Declare(
    opentims_testdata_dia
    URL "${OPENTIMS_TESTDATA_URL}/DIA_HeLa_50ng_5_6min.d.zip"
    URL_HASH SHA256=bacf98b472ed3a3cfef2c2595a726fc6c06f059a41c17d9035b2b864932b6f5d
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_Declare(
    opentims_testdata_fasta
    URL "${OPENTIMS_TESTDATA_URL}/human_sp.fasta"
    URL_HASH SHA256=b205ed9970fee62413e2f192c6fb82daab86e4a5897ef521f907ecbfcd7b3e63
    DOWNLOAD_NO_EXTRACT TRUE
  )
  FetchContent_MakeAvailable(opentims_testdata_dda opentims_testdata_dia opentims_testdata_fasta)
  set(OPENTIMS_DDA_TEST_DATA "${opentims_testdata_dda_SOURCE_DIR}" CACHE PATH "Path to DDA test .d directory")
  set(OPENTIMS_DIA_TEST_DATA "${opentims_testdata_dia_SOURCE_DIR}" CACHE PATH "Path to DIA test .d directory")
  set(OPENTIMS_TEST_FASTA "${opentims_testdata_fasta_SOURCE_DIR}/human_sp.fasta" CACHE PATH "Path to human Swiss-Prot FASTA for search tests")
endif()

if (ENABLE_THERMO_RAW_TESTS AND WITH_THERMO_RAW)
  include(FetchContent)
  # Angiotensin_AllScans.raw originally from PNNL-Comp-Mass-Spec/Thermo-Raw-File-Reader
  # (BSD-2-Clause, (c) 2018 Battelle Memorial Institute), commit 1d8ff309. Contains
  # 87 MS1 (FTMS Full ms) plus hundreds of MS2 scans with HCD and supplemental-
  # activation (ETD+HCD, ETD+CID) filters at precursor m/z 432.9 — exercises the
  # full activation-method switch in ThermoRawFile.cpp and MS1+MSn round-trip via
  # mzML. Served from the OpenMS infra mirror (see OPENTIMS_TESTDATA_URL note above
  # re: plain HTTP); URL_HASH below pins integrity/reproducibility.
  FetchContent_Declare(
    thermo_raw_testdata
    URL "http://archive.openms.de/openms/testfiles/Angiotensin_AllScans.raw"
    URL_HASH SHA256=3a0236f719e7c91e3c958f57f4e66ae422803ec3e6a997b9af4d2af395332b9f
    DOWNLOAD_NO_EXTRACT TRUE
  )
  FetchContent_MakeAvailable(thermo_raw_testdata)
  set(THERMO_RAW_TEST_DATA "${thermo_raw_testdata_SOURCE_DIR}/Angiotensin_AllScans.raw" CACHE PATH "Path to Thermo RAW test file")
endif()

# Optional end-to-end FAIMS-DIA integration fixture. A developer can either set
# THERMO_FAIMS_DIA_TEST_DATA explicitly to an existing local RAW file, or enable
# ENABLE_THERMO_FAIMS_DIA_TESTS to fetch the pinned public PRIDE fixture. The
# ~308 MB download remains opt-in and is not part of normal OpenMS builds/tests.
set(THERMO_FAIMS_DIA_TEST_DATA "${THERMO_FAIMS_DIA_TEST_DATA}" CACHE FILEPATH
    "Path to a Thermo FAIMS-DIA RAW file for integration testing")
if (ENABLE_THERMO_FAIMS_DIA_TESTS AND WITH_THERMO_RAW AND NOT THERMO_FAIMS_DIA_TEST_DATA)
  include(FetchContent)
  # Public single-CV FAIMS-DIA run from PRIDE PXD029902. The pinned hash fixes
  # the exact 308,209,820-byte input used by ThermoRawFile_test.
  FetchContent_Declare(
    thermo_faims_dia_testdata
    URL "https://ftp.pride.ebi.ac.uk/pride/data/archive/2023/08/PXD029902/20210213_WTC11_DIA_5CV.raw"
    URL_HASH SHA256=e98b2cc95d162b1dfd24cf003434ef0deef085d7c99ffc0fde695aec8bf236b1
    DOWNLOAD_NO_EXTRACT TRUE
  )
  FetchContent_MakeAvailable(thermo_faims_dia_testdata)
  set(THERMO_FAIMS_DIA_TEST_DATA
      "${thermo_faims_dia_testdata_SOURCE_DIR}/20210213_WTC11_DIA_5CV.raw"
      CACHE FILEPATH "Path to a Thermo FAIMS-DIA RAW file for integration testing" FORCE)
endif()

if (THERMO_FAIMS_DIA_TEST_DATA AND NOT EXISTS "${THERMO_FAIMS_DIA_TEST_DATA}")
  message(FATAL_ERROR
    "THERMO_FAIMS_DIA_TEST_DATA does not exist: ${THERMO_FAIMS_DIA_TEST_DATA}")
endif()

if ((ENABLE_THERMO_FAIMS_DIA_TESTS OR THERMO_FAIMS_DIA_TEST_DATA) AND NOT WITH_THERMO_RAW)
  message(WARNING
    "Thermo FAIMS-DIA integration testing was requested but WITH_THERMO_RAW=OFF; the test will not run.")
endif()

#------------------------------------------------------------------------------
# Extend module path with our modules
# (create an empty list if CMAKE_MODULE_PATH does not exist)
if(NOT CMAKE_MODULE_PATH)
	set(CMAKE_MODULE_PATH)
endif()
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")

#------------------------------------------------------------------------------
# OpenMS Version Information
#------------------------------------------------------------------------------

set(OPENMS_PACKAGE_VERSION_MAJOR "4")
set(OPENMS_PACKAGE_VERSION_MINOR "0")
set(OPENMS_PACKAGE_VERSION_PATCH "0")
set(OPENMS_PACKAGE_VERSION "${OPENMS_PACKAGE_VERSION_MAJOR}.${OPENMS_PACKAGE_VERSION_MINOR}.${OPENMS_PACKAGE_VERSION_PATCH}")

message(STATUS "Building OpenMS ${OPENMS_PACKAGE_VERSION}")
#------------------------------------------------------------------------------
# retrieve detailed information on the working copy (git)
include(GetGitRevisionDescription)
option(GIT_TRACKING "Embed Git version information in the core library; branch changes and commits regenerate build metadata." ON)
if (GIT_TRACKING)
  ## will configure an include file
  git_short_info(OPENMS_GIT_SHORT_REFSPEC OPENMS_GIT_SHORT_SHA1 OPENMS_GIT_LC_DATE)
endif()

if(NOT GIT_TRACKING)
  ## allow overwriting of the following variables in case we build release versions
  ## from a source package (e.g. on bioconda)
  if(NOT DEFINED OPENMS_GIT_SHORT_SHA1)
    set(OPENMS_GIT_SHORT_SHA1 "disabled")
  endif()
  if(NOT DEFINED OPENMS_GIT_SHORT_REFSPEC)
    set(OPENMS_GIT_SHORT_REFSPEC "disabled")
  endif()
  if(NOT DEFINED OPENMS_GIT_LC_DATE)
    set(OPENMS_GIT_LC_DATE "1970-01-01")
  endif()
  message(STATUS "  [CMake is not tracking Git commits and branching ('-D GIT_TRACKING=Off')]")
elseif(OPENMS_GIT_SHORT_REFSPEC STREQUAL "GIT-NOTFOUND" OR OPENMS_GIT_SHORT_REFSPEC STREQUAL "HEAD-HASH-NOTFOUND")
  set(OPENMS_GIT_SHORT_SHA1 "exported")
  set(OPENMS_GIT_SHORT_REFSPEC "exported")
  set(OPENMS_GIT_LC_DATE "1970-01-01")
  message(STATUS "  [CMake cannot track Git commits and branching, since Git or the .git folder was not found. ('-D GIT_TRACKING=On')]")
else()
  # everything found, print some status information
  message(STATUS "  - Repository revision ${OPENMS_GIT_SHORT_SHA1}")
  message(STATUS "  - Repository branch ${OPENMS_GIT_SHORT_REFSPEC}")
  message(STATUS "  - Repository last change date ${OPENMS_GIT_LC_DATE}")
  message(STATUS "  [CMake is tracking Git commits and branching ('-D GIT_TRACKING=On')]")
endif()

# Product versions are independent of branch names. The SDK records its exact source.
set(OPENMS_PACKAGE_VERSION_FULLSTRING "${OPENMS_PACKAGE_VERSION}")
set(OPENMS_PACKAGE_VERSION_PRERELEASE_IDENTIFIER "experimental")
set(OPENMS_SOURCE_REVISION "" CACHE STRING "Exact 40-character package source revision for archive builds")
if(NOT OPENMS_SOURCE_REVISION AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE OPENMS_SOURCE_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _openms_revision_result)
  endif()
endif()
string(LENGTH "${OPENMS_SOURCE_REVISION}" _openms_revision_length)
if(NOT _openms_revision_length EQUAL 40 OR OPENMS_SOURCE_REVISION MATCHES "[^0-9a-fA-F]")
  message(FATAL_ERROR "Provide OPENMS_SOURCE_REVISION as the exact package commit when building a source archive.")
endif()

#------------------------------------------------------------------------------
# Unsupported systems
#------------------------------------------------------------------------------
if (MINGW OR MSYS)
  message(FATAL_ERROR "MSYS and/or MinGW are not supported! Please use a Visual Studio environment! See Windows build instructions for further information!")
endif()

#------------------------------------------------------------------------------
# Setup CMAKE_PREFIX_PATH for finding external libs (contrib or system)
#------------------------------------------------------------------------------
if(OPENMS_USE_VCPKG)
  message(STATUS "Dependency provider: vcpkg (${CMAKE_TOOLCHAIN_FILE})")
else()
  message(STATUS "Dependency provider: system/contrib")
endif()
include(cmake/setup_lib_find_paths.cmake)

#------------------------------------------------------------------------------
# All the multi-threading stuff (OpenMP)
#------------------------------------------------------------------------------
include(cmake/multithreading.cmake)

#------------------------------------------------------------------------------
# Check architecture 32bit vs. 64bit
#------------------------------------------------------------------------------

if (CMAKE_SIZEOF_VOID_P MATCHES "8")
	set(OPENMS_64BIT_ARCHITECTURE 1 CACHE INTERNAL "Architecture-bits")
	message(STATUS "Architecture: 64 bit")
else()
	set(OPENMS_64BIT_ARCHITECTURE 0 CACHE INTERNAL "Architecture-bits")
	message(STATUS "Architecture: 32 bit")
endif()

# Force build type into the cache (needs to be set beforehand)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)

#------------------------------------------------------------------------------
# Project specific compiler flags
#------------------------------------------------------------------------------
# Fill this with compile flags that external projects should use as well
# for OpenMS internal flags (not promoted to external compiler flags) append to CMAKE_CXX_FLAGS
# see OpenMS/cmake/OpenMSConfig.cmake.in to see how its configured and used (i.e. as OPENMS_ADDCXX_FLAGS)
include(cmake/compiler_flags.cmake)

# Visibility settings
if (NOT CMAKE_CXX_VISIBILITY_PRESET)
  set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
  cmake_policy(SET CMP0063 NEW)
endif()
if (CMAKE_VISIBILITY_INLINES_HIDDEN)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
endif()

#------------------------------------------------------------------------------
# Enable STL debug mode (GCC only)
#------------------------------------------------------------------------------
option(STL_DEBUG "[GCC only] Enable STL-DEBUG mode (very slow)." OFF)
if(STL_DEBUG)
  include(cmake/stl_debug.cmake)
endif()

#------------------------------------------------------------------------------
# Add options for link time optimizations (LTO) aka interprocedural optimization
#------------------------------------------------------------------------------
option(ENABLE_IPO
	"Enable interprocedural optimizations (LTO)" OFF)
if(ENABLE_IPO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT result OUTPUT output)
	if(result)
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
	else()
		message(SEND_ERROR "IPO/LTO is not supported: ${output}")
	endif()
endif()

#------------------------------------------------------------------------------
# Enable clang compile time profiling
#------------------------------------------------------------------------------
option(CLANG_TIME_TRACE "[Clang 9+ only] Enable clang compile time profiling." OFF)
if(CLANG_TIME_TRACE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftime-trace")
endif()


#------------------------------------------------------------------------------
# Enable Coverage
#------------------------------------------------------------------------------
option(OPENMS_COVERAGE "[Clang/GCC only] Enable Coverage testing with gcov/lcov. Requires the Debug build type,
 is slower and uses a lot of disk space (>40GB). Enables the target OpenMS_coverage to generate
 a coverage report in html format." OFF)
if(OPENMS_COVERAGE)
  include(cmake/Coverage.cmake)
endif()

#------------------------------------------------------------------------------
# Enable usage statistics
#------------------------------------------------------------------------------
option(ENABLE_UPDATE_CHECK "Enable update check." ON)
if(ENABLE_UPDATE_CHECK)
  message(STATUS "Collection of usage statistics and update notifications enabled.")
  message(STATUS "  If you don't want this information to be transmitted to our update sever, you can:")
  message(STATUS "    - Switch the build variable ENABLE_UPDATE_CHECK to OFF to remove the functionality at build time.")
  message(STATUS "    - Set the environment variable OPENMS_DISABLE_UPDATE_CHECK to disable the functionality at runtime.")
endif()


#------------------------------------------------------------------------------
# Enable AddressSanitizer
#------------------------------------------------------------------------------
option(ADDRESS_SANITIZER "Enable AddressSanitizer mode (quite slow)." OFF)
include(${PROJECT_SOURCE_DIR}/cmake/AddressSanitizer.cmake)

#------------------------------------------------------------------------------
# we build shared libraries
set(BUILD_SHARED_LIBS true)

#------------------------------------------------------------------------------
# Host directory for referencing from subprojects
set(OPENMS_HOST_DIRECTORY "${PROJECT_SOURCE_DIR}")
set(OPENMS_HOST_BINARY_DIRECTORY "${PROJECT_BINARY_DIR}")

# The additional / is important to remove the last character from the path.
# Note that it does not matter if the OS uses / or \, because we are only
# saving the path size.
string(LENGTH "${PROJECT_SOURCE_DIR}/" CF_SOURCE_PATH_SIZE)
string(LENGTH "${PROJECT_BINARY_DIR}/" BINARY_PATH_SIZE)
# Since the path's length is used to cut away a path, and OPENMS_FILENAME might be used in configured files in the build tree
# , taking the convervative approach here, to make sure we do not cut away too much, no matter if origin is source or build tree
if (BINARY_PATH_SIZE LESS CF_SOURCE_PATH_SIZE)
  set(CF_SOURCE_PATH_SIZE ${BINARY_PATH_SIZE})
endif()

#------------------------------------------------------------------------------
# CMake Utitlities
#------------------------------------------------------------------------------
# include some OpenMS specific macros
include (${PROJECT_SOURCE_DIR}/cmake/build_system_macros.cmake)
# .. and some to ease the installation process
include (${PROJECT_SOURCE_DIR}/cmake/install_macros.cmake)
# .. and some to ease the installation process
include (${PROJECT_SOURCE_DIR}/cmake/add_library_macros.cmake)
# .. and some to ease the export process
include (${PROJECT_SOURCE_DIR}/cmake/export_macros.cmake)


#------------------------------------------------------------------------------
# Output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OPENMS_HOST_BINARY_DIRECTORY}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OPENMS_HOST_BINARY_DIRECTORY}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OPENMS_HOST_BINARY_DIRECTORY}/lib")
# Some alternative variable names for internal use
set(OPENMS_BINARY_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(OPENMS_SHARE_DIR "${PROJECT_SOURCE_DIR}/share/OpenMS")
set(OPENMS_WIN32_DLL_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

#------------------------------------------------------------------------------
# define installation subdirectories to allow for custom installations
# note that all those directories are below CMAKE_INSTALL_PREFIX
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_SHARE_DIR share/OpenMS/${OPENMS_PACKAGE_VERSION} CACHE PATH "Installation directory for shared data")
set(INSTALL_DOC_DIR share/doc CACHE PATH "Installation directory for documentation")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")

if(WIN32)
  set(DEF_INSTALL_CMAKE_DIR CMake)
  set(DEF_INSTALL_LIB_DIR bin)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/cmake/OpenMS)
  set(DEF_INSTALL_LIB_DIR lib)
endif()

set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")
set(INSTALL_LIB_DIR ${DEF_INSTALL_LIB_DIR} CACHE PATH "Installation directory for libraries")
# Library-relative RPATH permits SDK relocation without a build-directory fallback.
if(UNIX)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  if(NOT DEFINED CMAKE_INSTALL_RPATH)
    if(APPLE)
      set(CMAKE_INSTALL_RPATH "@loader_path")
    else()
      set(CMAKE_INSTALL_RPATH "$ORIGIN")
    endif()
  endif()
endif()

#------------------------------------------------------------------------------
# lib naming when building with msvc & convenience location of build.bat
# TODO only for msvc or generally on Windows?
if (MSVC)
	## use OpenMSd.dll etc. in debug mode
    if(NOT DEFINED CMAKE_DEBUG_POSTFIX)
        set(CMAKE_DEBUG_POSTFIX "d")
    endif()
    if(NOT ("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}"))
        file(COPY "${PROJECT_SOURCE_DIR}/tools/build.bat" DESTINATION "${PROJECT_BINARY_DIR}")
    endif()
endif()


# Find core dependencies here, so all projects have access to the same variables and
# do not have to re-search for the core components

include(cmake/cmake_findExternalLibs.cmake)

#------------------------------------------------------------------------------
if(WITH_ONNX)
  # 1. Tell CMake to look for the package ONLY if the switch is ON.
  if(OPENMS_USE_VCPKG)
    find_package(onnxruntime CONFIG REQUIRED)
  else()
    find_package(ONNXRuntime REQUIRED)
  endif() 
endif()


#------------------------------------------------------------------------------
# The actual OpenMS code (headers and source files)
#------------------------------------------------------------------------------
add_subdirectory(src)

#------------------------------------------------------------------------------
# Install the namespaced core SDK. No product sources are added here.
set(OPENMS_WITH_OPENSWATH ON)
if(DISABLE_OPENSWATH)
  set(OPENMS_WITH_OPENSWATH OFF)
endif()
if(NOT Boost_VERSION_STRING)
  set(Boost_VERSION_STRING "${Boost_VERSION}")
endif()
openms_export_targets()
install_export_targets()
message(STATUS "Configured OpenMS core SDK ${OPENMS_PACKAGE_VERSION} at ${OPENMS_SOURCE_REVISION}")
