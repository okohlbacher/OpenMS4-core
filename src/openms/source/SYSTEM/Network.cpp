// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Chris Bielow $
// $Authors: Chris Bielow $
// --------------------------------------------------------------------------

#include <OpenMS/SYSTEM/Network.h>

#include <OpenMS/CONCEPT/Exception.h>
#include <OpenMS/CONCEPT/LogStream.h>
#include <OpenMS/SYSTEM/NetworkGetRequest.h>

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <string>

using namespace std;

namespace OpenMS
{

namespace
{
  /// Collision suffixes .0, .1, ... tried before giving up; bounds the scan and the counter.
  constexpr int MAX_NAME_COLLISIONS = 10000;

  // Create a new file for the download under the URL's file name, or with a number suffix when that
  // name is taken. Each candidate is reserved by exclusive creation ("x"), which fails for any
  // existing entry, dangling links included: testing for existence and then opening let a
  // concurrent download pick the same name and truncate the other's file.
  std::FILE* createUnusedFile_(const std::string& url, const std::string& dest_folder, std::string& filename)
  {
    namespace fs = std::filesystem;
    // extract filename from URL path
    std::string path_part = url;
    auto query_pos = path_part.find('?');
    if (query_pos != std::string::npos) path_part = StringUtils::substr(path_part, 0, query_pos);
    auto frag_pos = path_part.find('#');
    if (frag_pos != std::string::npos) path_part = StringUtils::substr(path_part, 0, frag_pos);

    std::string basename = fs::path(path_part).filename().string();
    if (basename.empty()) basename = "download";

    for (int i = -1; i < MAX_NAME_COLLISIONS; ++i)
    {
      filename = dest_folder + "/" + (i < 0 ? basename : basename + "." + std::to_string(i));
      if (std::FILE* file = std::fopen(filename.c_str(), "wbx"))
      {
        return file;
      }
      if (errno != EEXIST)
      {
        return nullptr; // e.g. a missing folder or no permission: another name would not help
      }
    }
    return nullptr;
  }
} // anonymous namespace

void Network::downloadFile(const std::string& url, const std::string& download_folder)
{
  NetworkGetRequest query;
  query.setUrl(url);
  query.setTimeout(600); // 10 minutes timeout
  query.run();

  if (!query.hasError())
  {
    std::string folder = download_folder.empty() ? "./" : download_folder;
    std::string filename;
    std::FILE* file = createUnusedFile_(url, folder, filename);
    if (file == nullptr)
    {
      throw Exception::IOException(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
        "Failed to create an unused output file: " + filename);
    }
    const auto& data = query.getResponseBinary();
    const bool written = data.empty() || std::fwrite(data.data(), 1, data.size(), file) == data.size();
    if (std::fclose(file) != 0 || !written)
    {
      throw Exception::IOException(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
        "Failed to write downloaded data to: " + filename);
    }
    OPENMS_LOG_INFO << "Download of '" << url << "' successful." << endl;
    OPENMS_LOG_INFO << "Stored as '" << filename << "'." << endl;
  }
  else
  {
    std::string error = "Download of '" + url + "' failed!. Error: " + query.getErrorString() + '\n';
    throw Exception::IOException(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, error);
  }
}

} // namespace OpenMS
