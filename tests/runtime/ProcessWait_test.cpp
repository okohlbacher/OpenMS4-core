// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $

#include "../../src/openms/source/SYSTEM/ProcessWait.h"
#include <boost/version.hpp>
#if BOOST_VERSION >= 108800
#include <boost/process/v1/child.hpp>
namespace bp = boost::process::v1;
#else
#include <boost/process/child.hpp>
namespace bp = boost::process;
#endif
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
  if (argc == 2)
  {
    if (std::string(argv[1]) == "--sleep")
    {
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    return std::string(argv[1]) == "--fail" ? 7 : 0;
  }

  for (const auto* argument : {"--success", "--fail"})
  {
    bp::child child(argv[0], argument);
    if (!OpenMS::Internal::waitForProcess(child, std::chrono::seconds(5)) ||
        child.exit_code() != (std::string(argument) == "--fail" ? 7 : 0))
    {
      std::cerr << "Completed process did not retain its exit status\n";
      return 1;
    }
  }

  bp::child child(argv[0], "--sleep");
  const bool finished = OpenMS::Internal::waitForProcess(child, std::chrono::milliseconds(20));
  const bool still_running = child.running();
  child.terminate();
  child.wait();
  if (finished || !still_running)
  {
    std::cerr << "Timed wait did not return control with the process still running\n";
    return 2;
  }
  return 0;
}
