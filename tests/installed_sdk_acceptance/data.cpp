// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $
#include <OpenMS/SYSTEM/File.h>
#include <filesystem>
#include <iostream>
#if defined(_WIN32)
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

int main(int argc, char** argv)
{
  if (argc != 3) { return 2; }
  const auto expected_data = std::filesystem::canonical(argv[1]);
  const auto actual_data = std::filesystem::canonical(OpenMS::File::getOpenMSDataPath());
  if (actual_data != expected_data)
  {
    std::cerr << "Resolved data from " << actual_data << "; expected " << expected_data << '\n';
    return 1;
  }
  if (! std::filesystem::exists(OpenMS::File::find("CHEMISTRY/unimod.xml"))) { return 3; }

  // A moved SDK must load its own core library, even while the old prefix exists.
  std::filesystem::path library;
#if defined(_WIN32)
  HMODULE module = nullptr;
  if (! GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(&OpenMS::File::getOpenMSDataPath), &module))
  {
    return 4;
  }
  char path[32768];
  if (! GetModuleFileNameA(module, path, sizeof(path))) { return 4; }
  library = path;
#else
  Dl_info info {};
  if (! dladdr(reinterpret_cast<const void*>(&OpenMS::File::getOpenMSDataPath), &info) || ! info.dli_fname) { return 4; }
  library = info.dli_fname;
#endif
  const auto prefix = std::filesystem::canonical(argv[2]);
  const auto relative = std::filesystem::canonical(library).lexically_relative(prefix);
  if (relative.empty() || *relative.begin() == "..")
  {
    std::cerr << "Loaded core from " << library << "; expected it under " << prefix << '\n';
    return 5;
  }
  return 0;
}
