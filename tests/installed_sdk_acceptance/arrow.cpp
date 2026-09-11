// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $
#include <OpenMS/FORMAT/ParquetFile.h>
#include <arrow/io/file.h>
#include <filesystem>

int main()
{
  arrow::DoubleBuilder values;
  OpenMS::ParquetFile::appendOrThrow(values.Append(100.25), "mz");
  OpenMS::ParquetFile::appendOrThrow(values.Append(200.5), "mz");
  auto array = OpenMS::ParquetFile::finishArray(values, "mz");
  const auto table = arrow::Table::Make(arrow::schema({arrow::field("mz", arrow::float64())}), {array});
  const std::string filename = "sdk-roundtrip.parquet";
  OpenMS::ParquetFile::writeTable(table, filename);
  const auto infile = arrow::io::ReadableFile::Open(filename).ValueOrDie();
  const auto restored = OpenMS::ParquetFile::readTable(infile);
  OpenMS::ParquetFile::appendOrThrow(infile->Close(), "input file");
  const auto rows = OpenMS::ParquetFile::rowCount(filename);
  std::filesystem::remove(filename);
  if (rows != 2 || restored->num_columns() != 1) { return 1; }
  const auto column = OpenMS::ParquetFile::getColumn(restored, "mz");
  return OpenMS::ParquetFile::getDouble(column, 0, 0.0, false) == 100.25 && OpenMS::ParquetFile::getDouble(column, 1, 0.0, false) == 200.5 ? 0 : 2;
}
