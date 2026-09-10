# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
# Run a probe in a fresh process and require OpenMS's specific override rejection.
# Generic WILL_FAIL also accepts unrelated loader failures and cannot prove this.
cmake_minimum_required(VERSION 3.24)

foreach(required IN ITEMS PROBE_EXECUTABLE INVALID_OVERRIDE)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing required input: ${required}")
  endif()
endforeach()
if(NOT EXISTS "${PROBE_EXECUTABLE}")
  message(FATAL_ERROR "Probe executable does not exist: ${PROBE_EXECUTABLE}")
endif()
if(EXISTS "${INVALID_OVERRIDE}")
  message(FATAL_ERROR "The negative-test override must be an absent path: ${INVALID_OVERRIDE}")
endif()

# Set the environment in this script process, avoiding a cmake -E env intermediary
# that can turn signal/loader termination into an ordinary exit code.
set(ENV{OPENMS_DATA_PATH} "${INVALID_OVERRIDE}")
execute_process(
  COMMAND "${PROBE_EXECUTABLE}" ${PROBE_ARGUMENTS}
  RESULT_VARIABLE probe_result
  OUTPUT_VARIABLE probe_stdout
  ERROR_VARIABLE probe_stderr
  TIMEOUT 30)
if(NOT "${probe_result}" STREQUAL "1")
  message(FATAL_ERROR
    "Expected OpenMS override rejection with exit code 1; got '${probe_result}'.\n"
    "stdout:\n${probe_stdout}\nstderr:\n${probe_stderr}")
endif()

string(REPLACE "\r\n" "\n" probe_stderr "${probe_stderr}")
if(probe_stderr MATCHES "dyld\\[|Library not loaded:|Symbol not found:|error while loading shared libraries:|symbol lookup error:|Segmentation fault|Subprocess aborted")
  message(FATAL_ERROR "Probe failed in the runtime loader or crashed, not in the expected data-path check:\n${probe_stderr}")
endif()
foreach(expected IN ITEMS
    "OpenMS FATAL ERROR!\n  Cannot find shared data! OpenMS cannot function without it!\n"
    "The environment variable 'OPENMS_DATA_PATH' currently points to '${INVALID_OVERRIDE}', which is incorrect!"
    "Exiting now.")
  string(FIND "${probe_stderr}" "${expected}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR
      "Probe exited 1 for the wrong reason: missing expected OpenMS diagnostic '${expected}'.\n"
      "stdout:\n${probe_stdout}\nstderr:\n${probe_stderr}")
  endif()
endforeach()
message(STATUS "OpenMS explicitly rejected the invalid OPENMS_DATA_PATH override (exit 1 and exact diagnostic verified).")
