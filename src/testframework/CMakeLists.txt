# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
#
# --------------------------------------------------------------------------
# $Maintainer: Timo Sachsenberg $
# $Authors: Timo Sachsenberg $
# --------------------------------------------------------------------------

project("OpenMSTestFramework")
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

#------------------------------------------------------------------------------
# The class-test framework (ClassTest.h macros, FuzzyStringComparator,
# MacrosTest.h) as its own STATIC library: standard-library only, no libOpenMS
# headers or symbols, so the tests of every OpenMS library (incl. ones that do
# not link libOpenMS, like OpenSwathAlgo) can use it. Library-specific behavior
# is registered by the test projects (see openms/source/OpenMSTestSupport.cpp).

set(OpenMSTestFramework_sources
  source/CONCEPT/ClassTest.cpp
  source/CONCEPT/FuzzyStringComparator.cpp
)

set(OpenMSTestFramework_sources_h
  include/OpenMS/CONCEPT/ClassTest.h
  include/OpenMS/CONCEPT/ClassTestUtils.h
  include/OpenMS/CONCEPT/FuzzyStringComparator.h
  include/OpenMS/CONCEPT/MacrosTest.h
)

source_group("Source Files\\CONCEPT" FILES ${OpenMSTestFramework_sources})
source_group("Header Files\\OpenMS\\CONCEPT" FILES ${OpenMSTestFramework_sources_h})

add_library(OpenMSTestFramework STATIC ${OpenMSTestFramework_sources} ${OpenMSTestFramework_sources_h})

target_include_directories(OpenMSTestFramework PUBLIC
                           "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
                           "$<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>"
                           "$<INSTALL_INTERFACE:${INSTALL_CMAKE_DIR}/TestSupport/include>")

target_compile_features(OpenMSTestFramework PUBLIC cxx_std_23)
if(OPENMS_STL_DEBUG_ENABLED)
  target_compile_definitions(OpenMSTestFramework PUBLIC "$<$<CONFIG:Debug>:_GLIBCXX_DEBUG>")
endif()

openms_add_library_compiler_flags(OpenMSTestFramework)

#------------------------------------------------------------------------------
# TestSupport is an atomic optional development component. Its export is separate
# from OpenMSTargets, so installing only the library never creates dangling imports.
set_target_properties(OpenMSTestFramework PROPERTIES EXPORT_NAME TestFramework
  POSITION_INDEPENDENT_CODE ON)
add_library(OpenMS::TestFramework ALIAS OpenMSTestFramework)
if(OPENMS_BUILD_TEST_SUPPORT)
  install(TARGETS OpenMSTestFramework EXPORT OpenMSTestSupportTargets
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR} COMPONENT TestSupport)
  install(DIRECTORY include/OpenMS/ DESTINATION ${INSTALL_INCLUDE_DIR}/OpenMS
    COMPONENT TestSupport FILES_MATCHING PATTERN "*.h")
  install(EXPORT OpenMSTestSupportTargets NAMESPACE OpenMS::
    DESTINATION ${INSTALL_CMAKE_DIR} COMPONENT TestSupport)
  install(FILES "${OPENMS_HOST_DIRECTORY}/src/tests/class_tests/openms/source/OpenMSTestSupport.cpp"
    DESTINATION ${INSTALL_CMAKE_DIR}/TestSupport COMPONENT TestSupport)
  install(DIRECTORY "${OPENMS_HOST_DIRECTORY}/src/tests/class_tests/openms/include/OpenMS/"
    DESTINATION ${INSTALL_CMAKE_DIR}/TestSupport/include/OpenMS COMPONENT TestSupport)
  install(DIRECTORY "${OPENMS_HOST_DIRECTORY}/src/tests/class_tests/openms/data/"
    DESTINATION ${INSTALL_SHARE_DIR}/test-data/core COMPONENT TestSupport)
  configure_file("${OPENMS_HOST_DIRECTORY}/cmake/OpenMSTestSupportConfig.cmake.in"
    "${OPENMS_HOST_BINARY_DIRECTORY}/OpenMSTestSupportConfig.cmake" @ONLY)
  install(FILES "${OPENMS_HOST_BINARY_DIRECTORY}/OpenMSTestSupportConfig.cmake"
    DESTINATION ${INSTALL_CMAKE_DIR} COMPONENT TestSupport)
endif()
