// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: OpenMS Team $
// $Authors: OpenMS Team $
// --------------------------------------------------------------------------
#pragma once

namespace OpenMS
{
/// @brief Parameter metadata shared by file serializers and command-line consumers.
namespace ParamTags
{
  inline constexpr const char* TAG_OUTPUT_FILE = "output file";
  inline constexpr const char* TAG_INPUT_FILE = "input file";
  inline constexpr const char* TAG_OUTPUT_DIR = "output dir";
  inline constexpr const char* TAG_OUTPUT_PREFIX = "output prefix";
  inline constexpr const char* TAG_ADVANCED = "advanced";
  inline constexpr const char* TAG_REQUIRED = "required";
} // namespace ParamTags
} // namespace OpenMS
