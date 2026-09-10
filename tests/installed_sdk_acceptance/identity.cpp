// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $
#include <OpenMS/CONCEPT/VersionInfo.h>
#include <OpenMS/openms_package_version.h>
#include <atomic>
#include <barrier>
#include <fstream>
#include <iterator>
#include <thread>
#include <vector>

int main(int argc, char** argv)
{
  if (argc != 4) { return 1; }
  std::ifstream metadata(argv[1]);
  if (!metadata) { return 2; }
  std::string expected_json(std::istreambuf_iterator<char>{metadata}, {});
  if (!expected_json.empty() && expected_json.back() == '\n') { expected_json.pop_back(); }
  const std::string expected_revision = argv[2];
  const bool expected_dirty = std::string(argv[3]) == "1";
  // Start calls together in a fresh process; no ClassTest initializer warms these APIs.
  std::barrier start(16);
  std::atomic<bool> failed{false};
  std::vector<std::thread> workers;
  for (int i = 0; i < 16; ++i)
  {
    workers.emplace_back([&] {
      start.arrive_and_wait();
      for (int repeat = 0; repeat < 100; ++repeat)
      {
        if (OpenMS::VersionInfo::getVersion() != OPENMS_PACKAGE_VERSION ||
            OpenMS::VersionInfo::getVersionStruct() != OpenMS::VersionInfo::VersionDetails::create(OPENMS_PACKAGE_VERSION) ||
            OpenMS::VersionInfo::getTime().empty() ||
            OpenMS::VersionInfo::getSourceRevision() != expected_revision ||
            OpenMS::VersionInfo::isSourceDirty() != expected_dirty ||
            OpenMS::VersionInfo::getBuildInfo() != expected_json)
        {
          failed = true;
        }
      }
    });
  }
  for (auto& worker : workers) { worker.join(); }
  return failed ? 3 : 0;
}
