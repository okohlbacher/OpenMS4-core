# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause

# Darwin initializes a normal CMAKE_FIND_FRAMEWORK variable to FIRST. Prefer
# ordinary libraries (especially curl) unless the user or toolchain explicitly
# configured a cached preference, e.g. -DCMAKE_FIND_FRAMEWORK=ONLY.
if(APPLE AND NOT DEFINED CACHE{CMAKE_FIND_FRAMEWORK})
  set(CMAKE_FIND_FRAMEWORK LAST)
endif()
