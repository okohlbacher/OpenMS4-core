// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $
#include <OpenMS/SYSTEM/PathUtils.h>
#include <OpenMS/SYSTEM/File.h>
#include <filesystem>

int main(int argc, char** argv)
{
  if (argc != 2) { return 1; }
  const auto actual = OpenMS::File::getExecutablePath();
  if (actual.empty()) { return 2; }
  return std::filesystem::equivalent(OpenMS::to_path(actual), OpenMS::to_path(argv[1])) ? 0 : 3;
}
