# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
#
# --------------------------------------------------------------------------
# $Maintainer: Stephan Aiche, Chris Bielow $
# $Authors: Andreas Bertsch, Chris Bielow, Stephan Aiche $
# --------------------------------------------------------------------------

#------------------------------------------------------------------------------
# This cmake file enables the STL debug mode

set(OPENMS_STL_DEBUG_ENABLED OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # PUBLIC target propagation is added on OpenMS; internal targets share the mode.
  add_compile_definitions("$<$<CONFIG:Debug>:_GLIBCXX_DEBUG>")
  set(OPENMS_STL_DEBUG_ENABLED ON)
else()
  message(FATAL_ERROR "STL_DEBUG requires GCC and libstdc++; disable it with other compilers")
endif()
