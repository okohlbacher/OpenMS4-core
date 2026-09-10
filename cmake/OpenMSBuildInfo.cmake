# SPDX-License-Identifier: BSD-3-Clause
# Keep runtime identity and the installed artifact metadata byte-identical.
function(_openms_json_string output value)
  string(REPLACE "\\" "\\\\" value "${value}")
  string(REPLACE "\"" "\\\"" value "${value}")
  string(REPLACE "\n" "\\n" value "${value}")
  string(REPLACE "\r" "\\r" value "${value}")
  string(REPLACE "\t" "\\t" value "${value}")
  set(${output} "\"${value}\"" PARENT_SCOPE)
endfunction()

set(OPENMS_BUILD_INFO "{}")
string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" schema_version 1)
foreach(_pair IN ITEMS
    "source_revision;OPENMS_SOURCE_REVISION" "version;OPENMS_PACKAGE_VERSION"
    "system_name;CMAKE_SYSTEM_NAME" "system_processor;CMAKE_SYSTEM_PROCESSOR"
    "cxx_compiler_id;CMAKE_CXX_COMPILER_ID" "cxx_compiler_version;CMAKE_CXX_COMPILER_VERSION")
  list(GET _pair 0 _key)
  list(GET _pair 1 _variable)
  _openms_json_string(_value "${${_variable}}")
  string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" "${_key}" "${_value}")
endforeach()
string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" build_type "\"$<CONFIG>\"")
string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" cxx_standard 23)
foreach(_pair IN ITEMS "source_dirty;OPENMS_SOURCE_DIRTY" "shared_libs;BUILD_SHARED_LIBS"
    "class_testing_enabled;ENABLE_CLASS_TESTING")
  list(GET _pair 0 _key)
  list(GET _pair 1 _variable)
  set(_value false)
  if(${_variable})
    set(_value true)
  endif()
  string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" "${_key}" "${_value}")
endforeach()
string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" features "{}")
foreach(_pair IN ITEMS "hdf5;WITH_HDF5" "opentims;WITH_OPENTIMS" "thermo_raw;WITH_THERMO_RAW"
    "tdl;ENABLE_TDL" "onnx;WITH_ONNX" "openmp;OPENMP_FOUND"
    "wnetalign;WITH_WNETALIGN" "openswath;OPENMS_WITH_OPENSWATH")
  list(GET _pair 0 _key)
  list(GET _pair 1 _variable)
  set(_value false)
  if(${_variable})
    set(_value true)
  endif()
  string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" features "${_key}" "${_value}")
endforeach()
set(_stl_debug false)
if(OPENMS_STL_DEBUG_ENABLED)
  set(_stl_debug "$<IF:$<CONFIG:Debug>,true,false>")
endif()
# Generator expressions are replaced only after CMake's JSON operations finish.
_openms_json_string(_value "${OPENMS_STANDARD_LIBRARY}")
string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" standard_library "${_value}")
if(NOT DEFINED OPENMS_LIBSTDCXX_CXX11_ABI)
  set(OPENMS_LIBSTDCXX_CXX11_ABI null)
endif()
string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" libstdcxx_cxx11_abi "${OPENMS_LIBSTDCXX_CXX11_ABI}")
set(_value null)
if(OPENMS_MSVC_RUNTIME_LIBRARY AND NOT OPENMS_MSVC_RUNTIME_LIBRARY STREQUAL "null")
  _openms_json_string(_value "${OPENMS_MSVC_RUNTIME_LIBRARY}")
endif()
string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" msvc_runtime_library "${_value}")
string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" stl_debug "\"OPENMS_STL_DEBUG_VALUE\"")
string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" dependencies "{}")
if(NOT CURL_VERSION)
  set(CURL_VERSION "${CURL_VERSION_STRING}")
endif()
foreach(_pair IN ITEMS "boost;Boost_VERSION_STRING" "eigen;Eigen3_VERSION"
    "arrow;Arrow_VERSION" "parquet;Parquet_VERSION" "curl;CURL_VERSION"
    "openmp;OpenMP_CXX_VERSION")
  list(GET _pair 0 _key)
  list(GET _pair 1 _variable)
  string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" dependencies "${_key}" "{}")
  _openms_json_string(_value "${${_variable}}")
  string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" dependencies "${_key}" version "${_value}")
endforeach()
foreach(_pair IN ITEMS "arrow;OPENMS_ARROW_TARGET" "parquet;OPENMS_PARQUET_TARGET")
  list(GET _pair 0 _key)
  list(GET _pair 1 _variable)
  _openms_json_string(_value "${${_variable}}")
  string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" dependencies "${_key}" target "${_value}")
endforeach()
set(_boost_linkage shared)
if(BOOST_USE_STATIC)
  set(_boost_linkage static)
endif()
string(JSON OPENMS_BUILD_INFO SET "${OPENMS_BUILD_INFO}" dependencies boost linkage "\"${_boost_linkage}\"")
string(REPLACE "\"OPENMS_STL_DEBUG_VALUE\"" "${_stl_debug}" OPENMS_BUILD_INFO "${OPENMS_BUILD_INFO}")
file(GENERATE OUTPUT "${OPENMS_HOST_BINARY_DIRECTORY}/identity/$<CONFIG>/OpenMSBuildInfo.json"
  CONTENT "${OPENMS_BUILD_INFO}\n")
file(GENERATE OUTPUT "${OPENMS_HOST_BINARY_DIRECTORY}/identity/$<CONFIG>/OpenMS/openms_build_info.h"
  CONTENT "// Generated build identity; do not edit.\n#pragma once\n#define OPENMS_SOURCE_REVISION \"${OPENMS_SOURCE_REVISION}\"\n#define OPENMS_SOURCE_DIRTY $<BOOL:${OPENMS_SOURCE_DIRTY}>\n#define OPENMS_BUILD_INFO R\"OpenMS(${OPENMS_BUILD_INFO})OpenMS\"\n")
install(FILES "${OPENMS_HOST_BINARY_DIRECTORY}/identity/$<CONFIG>/OpenMSBuildInfo.json"
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT cmake)
add_custom_target(openms_source_identity
  COMMAND "${CMAKE_COMMAND}" "-DOPENMS_CHECK_SOURCE_DIR=${OPENMS_HOST_DIRECTORY}"
    "-DOPENMS_EXPECTED_REVISION=${OPENMS_SOURCE_REVISION}"
    "-DOPENMS_EXPECTED_DIRTY=${OPENMS_SOURCE_DIRTY}"
    "-DOPENMS_SOURCE_REVISION=${OPENMS_SOURCE_REVISION}"
    "-DOPENMS_SOURCE_DIRTY=${OPENMS_SOURCE_DIRTY}"
    "-DOPENMS_REQUIRE_CLEAN_SOURCE=${OPENMS_REQUIRE_CLEAN_SOURCE}"
    -P "${OPENMS_HOST_DIRECTORY}/cmake/OpenMSSourceIdentity.cmake"
  VERBATIM)
