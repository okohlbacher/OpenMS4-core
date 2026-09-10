# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
# 
# --------------------------------------------------------------------------
# $Maintainer: Stephan Aiche $
# $Authors: Stephan Aiche $
# --------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project("OpenMS_class_tests")

# Keep test executables in the common build/bin directory beside Core's DLLs.
# External dependency DLLs are supplied by the active dependency environment.
add_subdirectory(openswathalgo)
add_subdirectory(openms)
