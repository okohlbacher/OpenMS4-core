# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
#
# --------------------------------------------------------------------------
# $Maintainer: Julianus Pfeuffer $
# $Authors: Stephan Aiche, Chris Bielow, Julianus Pfeuffer $
# --------------------------------------------------------------------------

# required modules
include(CMakeParseArguments)
include(GenerateExportHeader)

#------------------------------------------------------------------------------
## export a single option indicating if libraries should be build as unity
## build
option(ENABLE_UNITYBUILD "Enables unity builds for all libraries." OFF)

#------------------------------------------------------------------------------
# openms_add_library()
# Create an OpenMS library, install it, register for export of targets, and
# export all required variables for later usage in the build system.
#
# Signature:
# openms_add_library(TARGET_NAME  OpenMS
#                    SOURCE_FILES  <source files to build the library>
#                    HEADER_FILES  <header files associated to the library>
#                                  (will be installed with the library)
#                    INTERNAL_INCLUDES <list of internal include directories for the library>
#                    PRIVATE_INCLUDES <list of include directories that will be used for compilation but that will not be exposed to other libraries>
#                    EXTERNAL_INCLUDES <list of external include directories for the library>
#                                      (will be added with -isystem if available)
#                    LINK_LIBRARIES <list of libraries used when linking the library>
#                    PRIVATE_LINK_LIBRARIES <list of internal libraries used when linking the library>
#                    DLL_EXPORT_PATH <path to the dll export header>)
function(openms_add_library)
  #------------------------------------------------------------------------------
  # parse arguments to function
  set(options )
  set(oneValueArgs TARGET_NAME DLL_EXPORT_PATH)
  set(multiValueArgs INTERNAL_INCLUDES PRIVATE_INCLUDES EXTERNAL_INCLUDES SOURCE_FILES HEADER_FILES LINK_LIBRARIES PRIVATE_LINK_LIBRARIES)
  ## make above arguments available as variables, e.g. ${openms_add_library_PRIVATE_LINK_LIBRARIES}
  cmake_parse_arguments(openms_add_library "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  #------------------------------------------------------------------------------
  # Status message for configure output
  message(STATUS "Adding library ${openms_add_library_TARGET_NAME}")

  #------------------------------------------------------------------------------
  # merge into global exported includes
  set(${openms_add_library_TARGET_NAME}_INCLUDE_DIRECTORIES ${openms_add_library_INTERNAL_INCLUDES}
                                                            ${openms_add_library_EXTERNAL_INCLUDES}
      CACHE INTERNAL "${openms_add_library_TARGET_NAME} include directories" FORCE)

  # CMake owns batching, generated files and per-source unity exclusions.
  add_library(${openms_add_library_TARGET_NAME} ${openms_add_library_SOURCE_FILES})
  if(ENABLE_UNITYBUILD)
    set_target_properties(${openms_add_library_TARGET_NAME} PROPERTIES UNITY_BUILD ON)
  endif()

  #------------------------------------------------------------------------------
  # Include directories
  # since internal includes all start with include/OpenMS and install_headers takes care of merging them in the install tree,
  # we can reference them just by INSTALL_INCLUDE_DIR in the install tree. They are then included as usual via <OpenMS/OPENSWATHALGO/..>"
  target_include_directories(${openms_add_library_TARGET_NAME} PUBLIC
                             "$<BUILD_INTERFACE:${openms_add_library_INTERNAL_INCLUDES}>"
                             "$<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>"  # <prefix>/include
                             )

  # TODO actually we shouldn't need to add these external includes. They should propagate through target_link_library if they are public
  target_include_directories(${openms_add_library_TARGET_NAME} SYSTEM PUBLIC 
                             "$<BUILD_INTERFACE:${openms_add_library_EXTERNAL_INCLUDES}>"
                             "$<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>"
                             )
  target_include_directories(${openms_add_library_TARGET_NAME} SYSTEM PRIVATE ${openms_add_library_PRIVATE_INCLUDES})
  
  if(OPENMS_STL_DEBUG_ENABLED)
    target_compile_definitions(${openms_add_library_TARGET_NAME} PUBLIC "$<$<CONFIG:Debug>:_GLIBCXX_DEBUG>")
  endif()

  # Add compiler flags using the new helper function
  openms_add_library_compiler_flags(${openms_add_library_TARGET_NAME})

  set_target_properties(${openms_add_library_TARGET_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
  set_target_properties(${openms_add_library_TARGET_NAME} PROPERTIES VISIBILITY_INLINES_HIDDEN 1)

  #------------------------------------------------------------------------------
  # Generate export header if requested
  if(NOT ${openms_add_library_DLL_EXPORT_PATH} STREQUAL "")
    ## this snipped creates 'OpenMSConfig.h' in the build tree
    set(_CONFIG_H "include/${openms_add_library_DLL_EXPORT_PATH}${openms_add_library_TARGET_NAME}Config.h")
    string(TOUPPER ${openms_add_library_TARGET_NAME} _TARGET_UPPER_CASE)
    generate_export_header(${openms_add_library_TARGET_NAME}
                          EXPORT_MACRO_NAME ${_TARGET_UPPER_CASE}_DLLAPI
                          EXPORT_FILE_NAME ${_CONFIG_H})

    string(REGEX REPLACE "/" "\\\\" _fixed_path ${openms_add_library_DLL_EXPORT_PATH})

    # add generated header to visual studio
    source_group("Header Files\\${_fixed_path}" FILES ${_CONFIG_H})
  endif()

  #------------------------------------------------------------------------------
  # Link library against other libraries
  if(openms_add_library_LINK_LIBRARIES)
    target_link_libraries(${openms_add_library_TARGET_NAME} PUBLIC ${openms_add_library_LINK_LIBRARIES})
  endif()

  if (openms_add_library_PRIVATE_LINK_LIBRARIES)
    target_link_libraries(${openms_add_library_TARGET_NAME} PRIVATE ${openms_add_library_PRIVATE_LINK_LIBRARIES})
  endif()

  #------------------------------------------------------------------------------
  # Export libraries (self + dependencies)
  set(${openms_add_library_TARGET_NAME}_LIBRARIES
        ${openms_add_library_TARGET_NAME}
        ${openms_add_library_LINK_LIBRARIES}
        CACHE
        INTERNAL "${openms_add_library_TARGET_NAME} libraries" FORCE)

  #------------------------------------------------------------------------------
  # we also want to install the library
  install_library(${openms_add_library_TARGET_NAME})
  install_headers("${openms_add_library_HEADER_FILES};${PROJECT_BINARY_DIR}/${_CONFIG_H}" ${openms_add_library_TARGET_NAME})

  #------------------------------------------------------------------------------
  # register for export
  openms_register_export_target(${openms_add_library_TARGET_NAME})

  #------------------------------------------------------------------------------
  # Status message for configure output
  message(STATUS "Adding library ${openms_add_library_TARGET_NAME} - SUCCESS")
endfunction()
