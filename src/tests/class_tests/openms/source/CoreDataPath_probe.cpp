// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: OpenMS Team $
// $Authors: OpenMS Team $
// --------------------------------------------------------------------------
#include <OpenMS/SYSTEM/File.h>
#include <filesystem>

int main(int argc, char** argv)
{
  if (argc != 2) { return 2; }
  // Resolve in a fresh process so each test checks the environment at first use.
  const auto resolved = std::filesystem::weakly_canonical(OpenMS::File::getOpenMSDataPath());
  return resolved == std::filesystem::weakly_canonical(argv[1]) ? 0 : 1;
}
