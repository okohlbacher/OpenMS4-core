# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
# $Maintainer: OpenMS Team $

cmake_minimum_required(VERSION 3.24)
project(ArrowDiagnosis LANGUAGES CXX)
find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
add_library(arrow_reader SHARED reader.cpp)
target_compile_definitions(arrow_reader PRIVATE PROBE_LIBRARY)
target_compile_features(arrow_reader PRIVATE cxx_std_23)
target_link_libraries(arrow_reader PRIVATE Arrow::arrow_shared Parquet::parquet_shared)
add_executable(arrow_probe main.cpp)
target_compile_features(arrow_probe PRIVATE cxx_std_23)
target_link_libraries(arrow_probe PRIVATE arrow_reader Arrow::arrow_shared Parquet::parquet_shared)
if(WIN32)
  target_link_libraries(arrow_probe PRIVATE dbghelp)
endif()
