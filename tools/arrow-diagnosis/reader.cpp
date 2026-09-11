// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $

#include "reader.h"
#include <arrow/io/file.h>
#include <parquet/arrow/reader.h>
#include <stdexcept>
std::shared_ptr<arrow::Table> read(const std::string& file, bool prebuffer)
{
  auto input = arrow::io::ReadableFile::Open(file).ValueOrDie();
  parquet::arrow::FileReaderBuilder builder;
  auto status = builder.Open(input);
  if (!status.ok()) throw std::runtime_error(status.ToString());
  parquet::ArrowReaderProperties properties;
  properties.set_pre_buffer(prebuffer);
  builder.properties(properties);
  auto reader = builder.Build().ValueOrDie();
  std::shared_ptr<arrow::Table> table;
  status = reader->ReadTable(&table);
  if (!status.ok()) throw std::runtime_error(status.ToString());
  return table;
}
