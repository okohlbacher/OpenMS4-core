// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: OpenMS Team $
// $Authors: OpenMS Team $
// --------------------------------------------------------------------------
#include <OpenMS/SYSTEM/File.h>
#include <filesystem>
#include <OpenMS/CONCEPT/Exception.h>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) try
{
  if (argc != 2 && argc != 3) { return 2; }
  if (argc == 3)
  {
    // A failed cached initialization must throw and be retryable, without process exit.
    bool caught = false;
    try { OpenMS::File::getOpenMSDataPath(); }
    catch (const OpenMS::Exception::FileNotFound&) { caught = true; }
    if (!caught) { return 3; }
#ifdef _WIN32
    if (_putenv_s("OPENMS_DATA_PATH", argv[1]) != 0) { return 4; }
#else
    if (setenv("OPENMS_DATA_PATH", argv[1], 1) != 0) { return 4; }
#endif
  }
  // Resolve in a fresh process so each test checks the environment at first use.
  const auto resolved = std::filesystem::weakly_canonical(OpenMS::File::getOpenMSDataPath());
  return resolved == std::filesystem::weakly_canonical(argv[1]) ? 0 : 1;
}

catch (const OpenMS::Exception::FileNotFound& error)
{
  std::cerr << error.getMessage() << "\nExiting now.\n";
  return 1;
}
