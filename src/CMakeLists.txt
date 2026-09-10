# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
# Core SDK: products consume the installed package in their own repositories.
add_subdirectory(openswathalgo)
add_subdirectory(openms)
if(ENABLE_CLASS_TESTING OR OPENMS_BUILD_TEST_SUPPORT)
  add_subdirectory(testframework)
endif()
if(ENABLE_CLASS_TESTING)
  add_subdirectory(tests)
endif()
