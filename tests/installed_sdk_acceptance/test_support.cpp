// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $
#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/TestFileValidation.h>
#include <OpenMS/CONCEPT/UniqueIdGenerator.h>
#include <OpenMS/FORMAT/FASTAFile.h>
#include <OpenMS/test_config.h>

START_TEST(InstalledSDKTestSupport, "$Id$")

START_SECTION(installed support initializes deterministic unique IDs)
{
  const auto first_id = OpenMS::UniqueIdGenerator::getUniqueId();
  OpenMS::UniqueIdGenerator::setSeed(2453440375);
  TEST_EQUAL(first_id, OpenMS::UniqueIdGenerator::getUniqueId());
}
END_SECTION

START_SECTION(installed fixture and configured test header are usable)
{
  std::vector<OpenMS::FASTAFile::FASTAEntry> entries;
  OpenMS::FASTAFile().load(OPENMS_GET_TEST_DATA_PATH("FASTAFile_test.fasta"), entries);
  TEST_EQUAL(entries.size(), 5);
  TEST_EQUAL(entries.at(0).identifier, "P68509|1433F_BOVIN");
}
END_SECTION

END_TEST
