# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
#
# --------------------------------------------------------------------------
# $Maintainer: Julianus Pfeuffer $
# $Authors: Stephan Aiche, Julianus Pfeuffer $
# --------------------------------------------------------------------------

# a collection of wrapper for install functions that allows easier usage
# throughout the OpenMS build system

set(OPENMS_EXPORT_SET "OpenMSTargets")

#------------------------------------------------------------------------------
# Installs the library lib_target_name and all its headers set via
# set_target_properties(lib_target_name PROPERTIES PUBLIC_HEADER ${headers})
#
# @param lib_target_name The target name of the library that should be installed
macro(install_library lib_target_name)
    install(TARGETS ${lib_target_name}
      EXPORT ${OPENMS_EXPORT_SET}
      LIBRARY DESTINATION ${INSTALL_LIB_DIR} COMPONENT library
      ARCHIVE DESTINATION ${INSTALL_LIB_DIR} COMPONENT library
      RUNTIME DESTINATION ${INSTALL_LIB_DIR} COMPONENT library
      )
endmacro()

#------------------------------------------------------------------------------
# Installs the given headers.
#
# @param header_list List of headers to install
macro(install_headers header_list component)
  foreach(_header ${header_list})
    # nlohmann::json is a PRIVATE dependency of libOpenMS: downstream builds do not have its
    # include directory, so no installed header may include it (configure-time check only).
    # Header lists are relative to the calling source directory, like install(FILES) resolves them.
    # Generated headers in the build tree may not exist yet at this point; they come from OpenMS'
    # own templates, so only what is already on disk is scanned.
    set(_header_to_scan "${_header}")
    if (NOT IS_ABSOLUTE "${_header_to_scan}")
      set(_header_to_scan "${CMAKE_CURRENT_SOURCE_DIR}/${_header_to_scan}")
    endif()
    if (EXISTS "${_header_to_scan}" AND NOT IS_DIRECTORY "${_header_to_scan}")
      file(STRINGS "${_header_to_scan}" _nlohmann_json_hits REGEX "nlohmann/json")
      if (_nlohmann_json_hits)
        message(FATAL_ERROR "Installed header ${_header} includes nlohmann/json. Keep JSON types out of "
                            "public headers: move the header under source/ or expose value types instead.")
      endif()
    endif()
    set(_relative_header_path)

    get_filename_component(_target_path ${_header} PATH)
    if ("${_target_path}" MATCHES "^${PROJECT_BINARY_DIR}.*")
      # is generated bin header
      string(REPLACE "${PROJECT_BINARY_DIR}/include/OpenMS" "" _relative_header_path "${_target_path}")
    else()
      # is source header -> strip include/OpenMS
      string(REPLACE "include/OpenMS" "" _relative_header_path "${_target_path}")
    endif()

    # install the header
    install(FILES ${_header}
            # note the missing slash, we need this for file directly located in
            # include/OpenMS (e.g., config.h)
            DESTINATION ${INSTALL_INCLUDE_DIR}/OpenMS${_relative_header_path}
            COMPONENT ${component}_headers)
  endforeach()
endmacro()

#------------------------------------------------------------------------------
# Installs a given directory
# @param directory The directory to install
# @param destination The destination (relative to the prefix) where it should be installed
# @param component The component to which to the directory belongs
macro(install_directory directory destination component)
    install(DIRECTORY ${directory}
      DESTINATION ${destination}
      COMPONENT ${component}
      FILE_PERMISSIONS      OWNER_WRITE OWNER_READ
                            GROUP_READ
                            WORLD_READ
      DIRECTORY_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                            GROUP_EXECUTE GROUP_READ
                            WORLD_EXECUTE WORLD_READ
      REGEX "^\\..*" EXCLUDE ## Exclude hidden files (svn, git, DSStore)
      REGEX ".*\\/\\..*" EXCLUDE ## Exclude hidden files in subdirectories
        )
endmacro()

#------------------------------------------------------------------------------
# Installs a given file
# @param directory The file to install
# @param destination The destination (relative to the prefix) where it should be installed
# @param component The component to which to the file belongs
macro(install_file file destination component)
    install(FILES ${file}
      DESTINATION ${destination}
      COMPONENT ${component})
endmacro()

#------------------------------------------------------------------------------
# Installs the exported target information
macro(install_export_targets )
    install(EXPORT ${OPENMS_EXPORT_SET}
            NAMESPACE OpenMS::
            DESTINATION ${INSTALL_CMAKE_DIR}
            COMPONENT cmake)
endmacro()
