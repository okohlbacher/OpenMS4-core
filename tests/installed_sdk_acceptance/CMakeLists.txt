# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.24)
project(OpenMSInstalledSDKAcceptance LANGUAGES CXX)

include(CTest)
option(OPENMS_ACCEPTANCE_TEST_SUPPORT "Validate the optional installed TestSupport component" ON)
set(OPENMS_EXPECTED_SOURCE_REVISION "" CACHE STRING "Expected full Core SDK source revision")
set(OPENMS_EXPECTED_PREFIX "" CACHE PATH "Installed SDK prefix being accepted")
if(NOT IS_DIRECTORY "${OPENMS_EXPECTED_PREFIX}")
  message(FATAL_ERROR "Pass OPENMS_EXPECTED_PREFIX for an installed Core SDK; a build-tree package is insufficient.")
endif()

if(OPENMS_ACCEPTANCE_TEST_SUPPORT)
  find_package(OpenMS 4.0.0 EXACT CONFIG REQUIRED COMPONENTS TestSupport)
else()
  find_package(OpenMS 4.0.0 EXACT CONFIG REQUIRED)
endif()
if(OPENMS_EXPECTED_SOURCE_REVISION AND
   NOT OpenMS_SOURCE_REVISION STREQUAL OPENMS_EXPECTED_SOURCE_REVISION)
  message(FATAL_ERROR "Core SDK source mismatch: ${OpenMS_SOURCE_REVISION}; expected ${OPENMS_EXPECTED_SOURCE_REVISION}")
endif()
foreach(name IN ITEMS Core OpenSwathAlgo Arrow)
  get_target_property(imported OpenMS::${name} IMPORTED)
  if(NOT imported)
    message(FATAL_ERROR "OpenMS::${name} must be an installed imported target.")
  endif()
endforeach()
foreach(forbidden IN ITEMS OpenMS::CLI OpenMS::GUI OpenMS_CLI OpenMS_GUI TOPPView FuzzyDiff)
  if(TARGET ${forbidden})
    message(FATAL_ERROR "Core SDK imported an application target: ${forbidden}")
  endif()
endforeach()
if(CMAKE_BUILD_TYPE AND OpenMS_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES AND
   NOT CMAKE_BUILD_TYPE STREQUAL OpenMS_BUILD_TYPE)
  message(FATAL_ERROR "Match the SDK build type ${OpenMS_BUILD_TYPE}; do not mix binary configurations.")
endif()

find_package(Threads REQUIRED)
add_executable(sdk_identity identity.cpp)
target_link_libraries(sdk_identity PRIVATE OpenMS::Core Threads::Threads)
add_test(NAME sdk_runtime_identity COMMAND sdk_identity "${OpenMS_BUILD_INFO_FILE}"
  "${OpenMS_SOURCE_REVISION}" "$<BOOL:${OpenMS_SOURCE_DIRTY}>")

foreach(probe IN ITEMS api eigen arrow data)
  add_executable(sdk_${probe} ${probe}.cpp)
  # Eigen and Arrow are deliberately not rediscovered or linked separately.
  # Their public headers/symbols must be supplied by the installed Core target.
  target_link_libraries(sdk_${probe} PRIVATE OpenMS::Core)
  target_compile_features(sdk_${probe} PRIVATE cxx_std_23)
endforeach()
target_link_libraries(sdk_data PRIVATE ${CMAKE_DL_LIBS})
foreach(probe IN ITEMS api eigen arrow)
  add_test(NAME sdk_${probe} COMMAND ${CMAKE_COMMAND} -E env --unset=OPENMS_DATA_PATH
    $<TARGET_FILE:sdk_${probe}>)
endforeach()

add_test(NAME sdk_library_relative_data COMMAND ${CMAKE_COMMAND} -E env --unset=OPENMS_DATA_PATH
  $<TARGET_FILE:sdk_data> "${OpenMS_DATA_DIR}" "${OPENMS_EXPECTED_PREFIX}")
set(override "${CMAKE_CURRENT_BINARY_DIR}/explicit-data")
file(MAKE_DIRECTORY "${override}/CHEMISTRY")
file(COPY "${OpenMS_DATA_DIR}/CHEMISTRY/unimod.xml" DESTINATION "${override}/CHEMISTRY")
add_test(NAME sdk_explicit_data_override COMMAND ${CMAKE_COMMAND} -E env
  "OPENMS_DATA_PATH=${override}" $<TARGET_FILE:sdk_data> "${override}" "${OPENMS_EXPECTED_PREFIX}")
add_test(NAME sdk_invalid_data_override COMMAND ${CMAKE_COMMAND}
  "-DPROBE_EXECUTABLE=$<TARGET_FILE:sdk_data>"
  "-DPROBE_ARGUMENTS=${OpenMS_DATA_DIR};${OPENMS_EXPECTED_PREFIX}"
  "-DINVALID_OVERRIDE=${CMAKE_CURRENT_BINARY_DIR}/missing-data-override"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/ExpectInvalidDataOverride.cmake")

if(OPENMS_ACCEPTANCE_TEST_SUPPORT)
  foreach(required IN ITEMS OpenMS_TEST_SUPPORT_SOURCE OpenMS_TEST_CONFIG_TEMPLATE OpenMS_TEST_DATA_DIR)
    if(NOT EXISTS "${${required}}")
      message(FATAL_ERROR "Installed TestSupport is incomplete: ${required}=${${required}}")
    endif()
  endforeach()
  set(CF_OPENMS_TEST_DATA_PATH "${OpenMS_TEST_DATA_DIR}/")
  configure_file("${OpenMS_TEST_CONFIG_TEMPLATE}" "include/OpenMS/test_config.h" @ONLY)
  add_executable(sdk_test_support test_support.cpp "${OpenMS_TEST_SUPPORT_SOURCE}")
  target_include_directories(sdk_test_support PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include" "${OpenMS_TEST_SUPPORT_INCLUDE_DIR}")
  target_link_libraries(sdk_test_support PRIVATE OpenMS::Core OpenMS::TestFramework)
  target_compile_features(sdk_test_support PRIVATE cxx_std_23)
  add_test(NAME sdk_test_support COMMAND ${CMAKE_COMMAND} -E env --unset=OPENMS_DATA_PATH
    $<TARGET_FILE:sdk_test_support>)
endif()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/accepted-sdk.txt"
  "version=${OpenMS_VERSION}\nrevision=${OpenMS_SOURCE_REVISION}\nprefix=${OPENMS_EXPECTED_PREFIX}\n"
  "data=${OpenMS_DATA_DIR}\narrow=${OpenMS_ARROW_VERSION}\nconfiguration=${OpenMS_BUILD_TYPE}\n")
