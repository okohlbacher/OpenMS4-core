// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// 
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg$
// $Authors: Timo Sachsenberg$
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/CONCEPT/LogStream.h>
#include <OpenMS/test_config.h>

///////////////////////////
#include <OpenMS/METADATA/ExperimentalDesign.h>
#include <OpenMS/FORMAT/ExperimentalDesignFile.h>
#include <OpenMS/FORMAT/TextFile.h>

#include <sstream>
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

START_SECTION((static ExperimentalDesign load(const TextFile&, bool, String) pads sample rows that lack trailing cells))
{
  // Sample rows are split before they are trimmed, so a blank last value that keeps its tab is a cell of its own
  // (S1, S2) and needs no padding. A row whose trailing tabs are missing, e.g. because an editor strips trailing
  // whitespace, has fewer cells than the sample header (S3, S4). It is padded with empty values.
  TextFile tf;
  tf.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample");
  tf.addLine("1\t1\ta.mzML\t1\tS1");
  tf.addLine("2\t1\tb.mzML\t1\tS2");
  tf.addLine("3\t1\tc.mzML\t1\tS3");
  tf.addLine("4\t1\td.mzML\t1\tS4");
  tf.addLine("");
  tf.addLine("Sample\tMSstats_Condition\tMSstats_BioReplicate");
  tf.addLine("S1\tA\t");
  tf.addLine("S2\t\t"); // both values blank
  tf.addLine("S3\tC");  // one cell short
  tf.addLine("S4");     // two cells short

  const auto test_design = [](const ExperimentalDesign& design)
  {
    const ExperimentalDesign::SampleSection& ss = design.getSampleSection();
    TEST_EQUAL(ss.getContentSize(), 4)
    TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Condition"), "A")
    TEST_EQUAL(ss.getFactorValue("S1", "MSstats_BioReplicate"), "")
    TEST_EQUAL(ss.getFactorValue(0, "MSstats_BioReplicate"), "")
    TEST_EQUAL(ss.getFactorValue("S2", "MSstats_Condition"), "")
    TEST_EQUAL(ss.getFactorValue("S2", "MSstats_BioReplicate"), "")
    TEST_EQUAL(ss.getFactorValue("S3", "MSstats_Condition"), "C")
    TEST_EQUAL(ss.getFactorValue("S3", "MSstats_BioReplicate"), "")
    TEST_EQUAL(ss.getFactorValue(2, "MSstats_BioReplicate"), "")
    TEST_EQUAL(ss.getFactorValue("S4", "MSstats_Condition"), "")
    TEST_EQUAL(ss.getFactorValue("S4", "MSstats_BioReplicate"), "")
    TEST_EQUAL(ss.getFactorValue(3, "MSstats_Condition"), "")
  };
  test_design(ExperimentalDesignFile::load(tf, false, "inline_short_sample_rows.tsv"));

  // the same from a file
  std::string filename;
  NEW_TMP_FILE(filename);
  tf.store(filename);
  test_design(ExperimentalDesignFile::load(filename, false));

  // a row that lacks the sample name itself is still an error: a blank name, and a row too short to reach the Sample column
  TextFile no_name;
  no_name.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample");
  no_name.addLine("1\t1\ta.mzML\t1\tS1");
  no_name.addLine("");
  no_name.addLine("MSstats_Condition\tSample");
  no_name.addLine("A\t");
  TEST_EXCEPTION(Exception::ParseError, ExperimentalDesignFile::load(no_name, false, "inline_missing_sample_name.tsv"))
  TextFile short_of_name;
  short_of_name.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample");
  short_of_name.addLine("1\t1\ta.mzML\t1\tS1");
  short_of_name.addLine("");
  short_of_name.addLine("MSstats_Condition\tSample");
  short_of_name.addLine("A");
  TEST_EXCEPTION(Exception::ParseError, ExperimentalDesignFile::load(short_of_name, false, "inline_short_of_sample_name.tsv"))
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

START_SECTION((static ExperimentalDesign load(const TextFile&, bool, String) aligns an indented sample table))
{
  // A spreadsheet with an empty first column exports every line of the sample table with a leading tab. The header
  // and the rows are split the same way, so the columns still line up; core-v4.0.0-ci.5 read such designs correctly.
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
  // loads @p tf and returns the warnings logged meanwhile
  const auto load_warning = [](const TextFile& tf, const std::string& name, ExperimentalDesign::SampleSection& ss)
  {
    OPENMS_LOG_WARN->clearCache();
    std::ostringstream captured;
    OPENMS_LOG_WARN.insert(captured);
    try
    {
      ss = ExperimentalDesignFile::load(tf, false, name).getSampleSection();
    }
    catch (...)
    {
      OPENMS_LOG_WARN.remove(captured);
      OPENMS_LOG_WARN->clearCache();
      throw;
    }
    OPENMS_LOG_WARN.remove(captured);
    OPENMS_LOG_WARN->clearCache();
    return captured.str();
  };

  ExperimentalDesign::SampleSection ss;
  TextFile indented = design_with({
    "\tMSstats_Condition\tMSstats_BioReplicate\tSample",
    "\tA\t1\tS1",
    "\tB\t2\tS2"});
  TEST_EQUAL(load_warning(indented, "inline_indented.tsv", ss), "")
  TEST_EQUAL(ss.getContentSize(), 2)
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Condition"), "A")
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_BioReplicate"), "1")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_Condition"), "B")
  TEST_EQUAL(ss.getFactors().count(""), 0)

  // the same from a file
  std::string filename;
  NEW_TMP_FILE(filename);
  indented.store(filename);
  ss = ExperimentalDesignFile::load(filename, false).getSampleSection();
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Condition"), "A")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_BioReplicate"), "2")

  // Sample in the first named column, two blank leading columns, blank trailing header cells, and a row whose
  // blank first value keeps its column
  const TextFile sample_first = design_with({
    "\t\tSample\tMSstats_Condition\tMSstats_BioReplicate\t\t",
    "\t\tS1\tA\t1\t\t",
    "\t\tS2\t\t2"});
  TEST_EQUAL(load_warning(sample_first, "inline_indented_sample_first.tsv", ss), "")
  TEST_EQUAL(ss.getContentSize(), 2)
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Condition"), "A")
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_BioReplicate"), "1")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_Condition"), "")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_BioReplicate"), "2")
  TEST_EQUAL(ss.getFactors().count(""), 0)

  // A row that is indented while its header is not has a value outside the header's columns. It is ignored, and a
  // warning says the row may be shifted: here the shift goes unnoticed otherwise, because the replicate column holds
  // the sample names, so the shifted row still names a known sample.
  const TextFile stray_tab = design_with({
    "MSstats_Condition\tMSstats_BioReplicate\tSample",
    "A\tS1\tS1",
    "\tB\tS2\tS2"});
  const std::string warning = load_warning(stray_tab, "inline_stray_tab.tsv", ss);
  TEST_EQUAL(warning.find("row 2 of the sample table in 'inline_stray_tab.tsv' has values outside the columns of its header") != std::string::npos, true)
  TEST_EQUAL(ss.getFactorValue("S1", "MSstats_Condition"), "A")
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_BioReplicate"), "B")
  // an extra value after the last column is ignored with the same warning, and blank extra cells without one
  const TextFile extra_cells = design_with({
    "MSstats_Condition\tSample",
    "A\tS1\t\t",
    "B\tS2\tnote"});
  const std::string extra_warning = load_warning(extra_cells, "inline_extra_cells.tsv", ss);
  TEST_EQUAL(extra_warning.find("row 1 of") == std::string::npos, true)
  TEST_EQUAL(extra_warning.find("row 2 of the sample table in 'inline_extra_cells.tsv'") != std::string::npos, true)
  TEST_EQUAL(ss.getFactorValue("S2", "MSstats_Condition"), "B")
}
END_SECTION

START_SECTION((static ExperimentalDesign load(const TextFile&, bool, String) names a file-section sample that the sample section lacks))
{
  // This used to escape as a bare std::out_of_range ("map::at") that did not say which sample was missing.
  TextFile undefined;
  undefined.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample");
  undefined.addLine("1\t1\ta.mzML\t1\tS1");
  undefined.addLine("2\t1\tb.mzML\t1\tS3");
  undefined.addLine("3\t1\tc.mzML\t1\tS4");
  undefined.addLine("");
  undefined.addLine("Sample\tMSstats_Condition");
  undefined.addLine("S1\tA");
  undefined.addLine("S2\tB");
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, ExperimentalDesignFile::load(undefined, false, "inline_undefined_sample.tsv"),
    "Error: Sample 'S3' of the MS file section is missing from the sample section in: inline_undefined_sample.tsv")

  // the same from a file
  std::string filename;
  NEW_TMP_FILE(filename);
  undefined.store(filename);
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, ExperimentalDesignFile::load(filename, false),
    "Error: Sample 'S3' of the MS file section is missing from the sample section in: " + filename)

  // a blank Sample cell inside a file-section row names no sample of the sample section either
  TextFile blank;
  blank.addLine("Fraction_Group\tFraction\tSample\tSpectra_Filepath");
  blank.addLine("1\t1\t\ta.mzML");
  blank.addLine("");
  blank.addLine("Sample\tMSstats_Condition");
  blank.addLine("S1\tA");
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, ExperimentalDesignFile::load(blank, false, "inline_blank_file_sample.tsv"),
    "Error: Sample '' of the MS file section is missing from the sample section in: inline_blank_file_sample.tsv")

  // without a Sample column the file section names its samples after the fraction groups
  TextFile no_sample_column;
  no_sample_column.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel");
  no_sample_column.addLine("1\t1\ta.mzML\t1");
  no_sample_column.addLine("");
  no_sample_column.addLine("Sample\tMSstats_Condition");
  no_sample_column.addLine("S1\tA");
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, ExperimentalDesignFile::load(no_sample_column, false, "inline_no_sample_column.tsv"),
    "Error: Sample 'Fraction group 1' of the MS file section is missing from the sample section"
    " (the file section has no Sample column, so it names each sample after its fraction group) in: inline_no_sample_column.tsv")

  // which still loads if the sample section uses those names
  no_sample_column.addLine("Fraction group 1\tB");
  const ExperimentalDesign design = ExperimentalDesignFile::load(no_sample_column, false, "inline_fraction_group_names.tsv");
  TEST_EQUAL(design.getMSFileSection().size(), 1)
  TEST_EQUAL(design.getMSFileSection()[0].sample, 1)
  TEST_EQUAL(design.getSampleSection().getFactorValue("Fraction group 1", "MSstats_Condition"), "B")
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
