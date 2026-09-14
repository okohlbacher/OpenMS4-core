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
