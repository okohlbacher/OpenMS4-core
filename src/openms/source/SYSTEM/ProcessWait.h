// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $

#pragma once

#include <algorithm>
#include <chrono>
#include <thread>

namespace OpenMS::Internal
{
  // Boost.Process v1's timed waits are deprecated as unreliable. Poll its
  // nonblocking status with a monotonic deadline, then collect the exit status.
  template <typename Process>
  bool waitForProcess(Process& child, std::chrono::milliseconds timeout)
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (child.running())
    {
      const auto now = std::chrono::steady_clock::now();
      if (now >= deadline)
      {
        return false;
      }
      std::this_thread::sleep_until(std::min(deadline, now + std::chrono::milliseconds(10)));
    }
    child.wait();
    return true;
  }
}
