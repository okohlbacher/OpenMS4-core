// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $
#include <OpenMS/FORMAT/ParquetFile.h>
#include <filesystem>
#include <exception>
#include <iostream>

int main()
{
  try
  {
    arrow::DoubleBuilder values;
    OpenMS::ParquetFile::appendOrThrow(values.Append(100.25), "mz");
    OpenMS::ParquetFile::appendOrThrow(values.Append(200.5), "mz");
    auto array = OpenMS::ParquetFile::finishArray(values, "mz");
    const auto table = arrow::Table::Make(arrow::schema({arrow::field("mz", arrow::float64())}), {array});
    const std::string filename = "sdk-roundtrip.parquet";
    OpenMS::ParquetFile::writeTable(table, filename);
    const auto restored = OpenMS::ParquetFile::readTable(filename);
    const auto rows = OpenMS::ParquetFile::rowCount(filename);
    std::filesystem::remove(filename);
    if (rows != 2 || restored->num_columns() != 1) { return 1; }
    const auto column = OpenMS::ParquetFile::getColumn(restored, "mz");
    const bool matches = OpenMS::ParquetFile::getDouble(column, 0, 0.0, false) == 100.25 && OpenMS::ParquetFile::getDouble(column, 1, 0.0, false) == 200.5;
    if (!matches) { return 2; }
  }
  catch (const std::exception& error)
  {
    std::cerr << "Parquet SDK probe: " << error.what() << '\n';
    return 3;
  }
  catch (...)
  {
    std::cerr << "Parquet SDK probe: unknown exception\n";
    return 4;
  }
  return 0;
}
