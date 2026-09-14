// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// 
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg$
// $Authors: Timo Sachsenberg$
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>

///////////////////////////
#include <OpenMS/METADATA/ExperimentalDesign.h>
#include <OpenMS/FORMAT/ExperimentalDesignFile.h>
#include <OpenMS/FORMAT/TextFile.h>
///////////////////////////

using namespace OpenMS;
using namespace std;

START_TEST(ExperimentalDesignFile, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

ExperimentalDesignFile* ptr = 0;
ExperimentalDesignFile* null_ptr = 0;
START_SECTION(ExperimentalDesignFile())
{
  ptr = new ExperimentalDesignFile();
  TEST_NOT_EQUAL(ptr, null_ptr)
}
END_SECTION

START_SECTION(~ExperimentalDesignFile())
{
  delete ptr;
}
END_SECTION

START_SECTION((static ExperimentalDesign load(const std::string &tsv_file, bool require_spectra_files)))
{
ExperimentalDesign design = ExperimentalDesignFile::load(
  OPENMS_GET_TEST_DATA_PATH("ExperimentalDesign_input_1.tsv"), false);
  // tested in ExperimentalDesign_test
}
END_SECTION

START_SECTION((static ExperimentalDesign load(const TextFile&, bool, String) rejects multiplex one-table design without Sample column))
{
  TextFile tf;
  tf.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tMSstats_Condition");
  tf.addLine("1\t1\tmix_a.mzML\t1\tA");
  tf.addLine("1\t1\tmix_a.mzML\t2\tA");

  TEST_EXCEPTION(Exception::ParseError, ExperimentalDesignFile::load(tf, false, "inline_multiplex_no_sample.tsv"));
}
END_SECTION

START_SECTION((static ExperimentalDesign load(const TextFile&, bool, String) pads sample rows whose last values are blank))
{
  // A spreadsheet export with an optional factor left empty: the line is trimmed before it is split,
  // so the S1 row has one cell less than the sample header.
  TextFile tf;
  tf.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample");
  tf.addLine("1\t1\ta.mzML\t1\tS1");
  tf.addLine("2\t1\tb.mzML\t1\tS2");
  tf.addLine("");
  tf.addLine("Sample\tMSstats_Condition\tMSstats_BioReplicate");
  tf.addLine("S1\tA\t");
  tf.addLine("S2\t\t"); // both values blank

  const ExperimentalDesign design = ExperimentalDesignFile::load(tf, false, "inline_blank_last_value.tsv");
  const ExperimentalDesign::SampleSection& ss = design.getSampleSection();
  TEST_EQUAL(ss.getContentSize(), 2)
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Condition"), "A")
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_BioReplicate"), "")
  TEST_EQUAL(ss.getFactorValue(0, "MSstats_BioReplicate"), "")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_Condition"), "")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_BioReplicate"), "")

  // a row that lacks the sample name itself is still an error
  TextFile no_name;
  no_name.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample");
  no_name.addLine("1\t1\ta.mzML\t1\tS1");
  no_name.addLine("");
  no_name.addLine("MSstats_Condition\tSample");
  no_name.addLine("A\t");
  TEST_EXCEPTION(Exception::ParseError, ExperimentalDesignFile::load(no_name, false, "inline_missing_sample_name.tsv"))
}
END_SECTION

START_SECTION((static ExperimentalDesign load(const TextFile&, bool, String) keeps blank sample cells in their column))
{
  // A blank FIRST cell must not shift the row: its tab is whitespace, so trimming the line before
  // splitting it would drop the cell and move every later value one column to the left.
  const std::vector<std::string> file_section = {
    "Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample",
    "1\t1\ta.mzML\t1\tS1",
    "2\t1\tb.mzML\t1\tS2",
    ""};
  const auto design_with = [&file_section](const std::vector<std::string>& sample_section)
  {
    TextFile tf;
    for (const std::string& l : file_section) tf.addLine(l);
    for (const std::string& l : sample_section) tf.addLine(l);
    return tf;
  };

  // blank factor before the Sample column, and blank values at both ends of a row
  const TextFile before_sample = design_with({
    "MSstats_Condition\tSample\tMSstats_BioReplicate",
    "\tS1\t1",
    "B\tS2\t2"});
  ExperimentalDesign::SampleSection ss = ExperimentalDesignFile::load(before_sample, false, "inline_blank_first_cell.tsv").getSampleSection();
  TEST_EQUAL(ss.getContentSize(), 2)
  TEST_EQUAL(ss.hasSample("S1"), true)
  TEST_EQUAL(ss.hasSample("1"), false)
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Condition"), "")
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_BioReplicate"), "1")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_Condition"), "B")

  const TextFile both_ends = design_with({
    "MSstats_Condition\tMSstats_Fraction\tSample\tAlias",
    "\tX\tS1\t",
    "A\t\tS2\tsecond"});
  ss = ExperimentalDesignFile::load(both_ends, false, "inline_blank_outer_cells.tsv").getSampleSection();
  TEST_EQUAL(ss.getContentSize(), 2)
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Condition"), "")
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Fraction"), "X")
  TEST_EQUAL(ss.getFactorValue("S1", "Alias"), "")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_Condition"), "A")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_Fraction"), "")
  TEST_EQUAL(ss.getFactorValue("S2", "Alias"), "second")

  // the same from a file: loading it must not trim the lines either
  std::string filename;
  NEW_TMP_FILE(filename);
  design_with({
    "MSstats_Condition\tMSstats_Fraction\tSample\tAlias",
    "\tX\tS1\t",
    "A\t\tS2\tsecond"}).store(filename);
  ss = ExperimentalDesignFile::load(filename, false).getSampleSection();
  TEST_EQUAL(ss.getContentSize(), 2)
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Condition"), "")
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Fraction"), "X")
  TEST_EQUAL(ss.getFactorValue("S1", "Alias"), "")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_Condition"), "A")

  // a blank sample name is an error, wherever the Sample column is
  TextFile blank_first_name = design_with({
    "Sample\tMSstats_Condition\tMSstats_BioReplicate",
    "S1\tA\t1",
    "\tC\t3"});
  TEST_EXCEPTION(Exception::ParseError, ExperimentalDesignFile::load(blank_first_name, false, "inline_blank_sample_name.tsv"))
  const TextFile blank_inner_name = design_with({
    "MSstats_Condition\tSample\tMSstats_BioReplicate",
    "A\tS1\t1",
    "B\t\t2"});
  TEST_EXCEPTION(Exception::ParseError, ExperimentalDesignFile::load(blank_inner_name, false, "inline_blank_inner_sample_name.tsv"))
  std::string blank_name_file;
  NEW_TMP_FILE(blank_name_file);
  blank_first_name.store(blank_name_file);
  TEST_EXCEPTION(Exception::ParseError, ExperimentalDesignFile::load(blank_name_file, false))
}
END_SECTION

START_SECTION((static ExperimentalDesign load(const TextFile&, bool, String) ignores extra cells in sample rows))
{
  TextFile tf;
  tf.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample");
  tf.addLine("1\t1\ta.mzML\t1\tS1");
  tf.addLine("2\t1\tb.mzML\t1\tS2");
  tf.addLine("");
  tf.addLine("Sample\tMSstats_Condition\tMSstats_BioReplicate");
  tf.addLine("S1\tA\t1\tnote without a header");
  tf.addLine("S2\tB\t2");

  const ExperimentalDesign design = ExperimentalDesignFile::load(tf, false, "inline_extra_cell.tsv");
  const ExperimentalDesign::SampleSection& ss = design.getSampleSection();
  TEST_EQUAL(ss.getContentSize(), 2)
  TEST_EQUAL(ss.getFactors().size(), 3)
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Condition"), "A")
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_BioReplicate"), "1")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_BioReplicate"), "2")
}
END_SECTION

START_SECTION((static ExperimentalDesign load(const TextFile&, bool, String) rejects one-table rows of the wrong width))
{
  // Label, Fraction and Fraction_Group are read from the row, so a short row must be rejected before that.
  TextFile tf;
  tf.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample");
  tf.addLine("1\t1\ta.mzML");
  TEST_EXCEPTION(Exception::ParseError, ExperimentalDesignFile::load(tf, false, "inline_short_row.tsv"))

  // label-free with an empty last value: the row is one cell short, and Label is appended after it
  TextFile label_free;
  label_free.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tSample\tMSstats_Condition");
  label_free.addLine("1\t1\ta.mzML\tS1\t");
  TEST_EXCEPTION(Exception::ParseError, ExperimentalDesignFile::load(label_free, false, "inline_short_label_free_row.tsv"))

  // one-table rows must match the header exactly, as before
  TextFile long_row;
  long_row.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample");
  long_row.addLine("1\t1\ta.mzML\t1\tS1\textra");
  TEST_EXCEPTION(Exception::ParseError, ExperimentalDesignFile::load(long_row, false, "inline_long_row.tsv"))
}
END_SECTION


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST
