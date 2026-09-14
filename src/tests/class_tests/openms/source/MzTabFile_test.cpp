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
#include <OpenMS/FORMAT/MzTabFile.h>
#include <OpenMS/FORMAT/MzTab.h>
#include <OpenMS/FORMAT/TextFile.h>
///////////////////////////

#include <algorithm>

using namespace OpenMS;
using namespace std;

class MzTabFile2 : public MzTabFile
{
  public:
    std::string generateMzTabPSMSectionRow2_(const MzTabPSMSectionRow& row, const vector<std::string>& optional_columns, const MzTabMetaData& meta) const
    {
      size_t n_columns = 0;
      return generateMzTabSectionRow_(row, optional_columns, meta, n_columns);
    }
};

START_TEST(MzTabFile, "$Id$")

/////////////////////////////////////////////////////////////

MzTabFile* ptr = nullptr;
MzTabFile* null_ptr = nullptr;

START_SECTION(MzTabFile())
{
  ptr = new MzTabFile();
  TEST_NOT_EQUAL(ptr, null_ptr)
}
END_SECTION

START_SECTION(void load(const std::string& filename, MzTab& mzTab) )
  MzTab mzTab;
  MzTabFile().load(OPENMS_GET_TEST_DATA_PATH("MzTabFile_SILAC.mzTab"), mzTab);
END_SECTION

START_SECTION(void store(const std::string& filename, MzTab& mzTab) )
{
  std::vector<std::string> files_to_test;
  files_to_test.push_back("MzTabFile_SILAC.mzTab");
  files_to_test.push_back("MzTabFile_SILAC2.mzTab");
  files_to_test.push_back("MzTabFile_labelfree.mzTab");
  files_to_test.push_back("MzTabFile_iTRAQ.mzTab");
  files_to_test.push_back("MzTabFile_Cytidine.mzTab");

  for (std::vector<std::string>::const_iterator sit = files_to_test.begin(); sit != files_to_test.end(); ++sit)
  {
    // load mzTab
    MzTab mzTab;
    MzTabFile().load(OPENMS_GET_TEST_DATA_PATH(*sit), mzTab);

    // store mzTab
    std::string stored_mzTab;
    NEW_TMP_FILE(stored_mzTab)
    MzTabFile().store(stored_mzTab, mzTab);

    // compare original and stored mzTab (discarding row order and spaces)
    TextFile file1;
    TextFile file2;
    file1.load(stored_mzTab);
    file2.load(OPENMS_GET_TEST_DATA_PATH(*sit));
    std::sort(file1.begin(), file1.end());
    std::sort(file2.begin(), file2.end());

    for (TextFile::Iterator it = file1.begin(); it != file1.end(); ++it)
    {
      StringUtils::substitute(*it, " ", "");
    }

    for (TextFile::Iterator it = file2.begin(); it != file2.end(); ++it)
    {
      StringUtils::substitute(*it, " ", "");
    }

    std::string tmpfile1;
    std::string tmpfile2;
    NEW_TMP_FILE(tmpfile1)
    NEW_TMP_FILE(tmpfile2)
    file1.store(tmpfile1);
    file2.store(tmpfile2);
    TEST_FILE_SIMILAR(tmpfile1.c_str(), tmpfile2.c_str())
  }
}
END_SECTION

START_SECTION(~MzTabFile())
{
  delete ptr;
}
END_SECTION

START_SECTION(generateMzTabPSMSectionRow_(const MzTabPSMSectionRow& row, const vector<std::string>& optional_columns) const)
{
  MzTabFile2 mzTab;
  MzTabPSMSectionRow row;
  MzTabOptionalColumnEntry e;
  MzTabString s;
  
  row.sequence.fromCellString("NDYKAPPQPAPGK");
  row.PSM_ID.fromCellString("38");
  row.accession.fromCellString("IPI:B1");
  row.unique.fromCellString("1");
  row.database.fromCellString("null");
  row.database_version.fromCellString("null");
  row.search_engine.fromCellString("[, , Percolator, ]");
  row.search_engine_score[0].fromCellString("51.9678841193106");
  
  e.first = "Percolator_score";
  s.fromCellString("0.359083");
  e.second = s;
  row.opt_.push_back(e);
  
  e.first = "Percolator_qvalue";
  s.fromCellString("0.00649874");  
  e.second = s;
  row.opt_.push_back(e);
  
  e.first = "Percolator_PEP";
  s.fromCellString("0.0420992");  
  e.second = s;
  row.opt_.push_back(e);
  
  e.first = "search_engine_sequence";
  s.fromCellString("NDYKAPPQPAPGK");  
  e.second = s;
  row.opt_.push_back(e);
  
  // Tests ///////////////////////////////  
  vector<std::string> optional_columns;
  optional_columns.push_back("Percolator_score");
  optional_columns.push_back("Percolator_qvalue");
  optional_columns.push_back("EMPTY");
  optional_columns.push_back("Percolator_PEP");
  optional_columns.push_back("search_engine_sequence");
  optional_columns.push_back("AScore_1");

  MzTabMetaData m{};
    
  std::string strRow(mzTab.generateMzTabPSMSectionRow2_(row, optional_columns, m));
  std::vector<std::string> substrings;
  StringUtils::split(strRow, '\t', substrings);
  TEST_EQUAL(substrings[substrings.size() - 1],"null")
  TEST_EQUAL(substrings[substrings.size() - 2],"NDYKAPPQPAPGK")
  TEST_EQUAL(substrings[substrings.size() - 3],"0.0420992")
  TEST_EQUAL(substrings[substrings.size() - 4],"null")
}
END_SECTION

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
START_SECTION(([EXTRA] optional PSM columns retain trailing empty cells as null))
  TextFile text(OPENMS_GET_TEST_DATA_PATH("MzTabFile_SILAC.mzTab"));
  Size psm_count = 0;
  for (auto& line : text)
  {
    if (StringUtils::hasPrefix(line, "PSH\t"))
    {
      line += "\topt_global_present\topt_global_empty";
    }
    else if (StringUtils::hasPrefix(line, "PSM\t"))
    {
      line += "\tkept\t";
      ++psm_count;
    }
  }
  TEST_NOT_EQUAL(psm_count, 0)
  std::string filename;
  NEW_TMP_FILE(filename)
  text.store(filename);
  MzTab loaded;
  MzTabFile().load(filename, loaded);
  TEST_EQUAL(loaded.getPSMSectionRows().size(), psm_count)
  for (const auto& row : loaded.getPSMSectionRows())
  {
    bool present = false;
    bool empty = false;
    for (const auto& column : row.opt_)
    {
      if (column.first == "opt_global_present") present = column.second.toCellString() == "kept";
      if (column.first == "opt_global_empty") empty = column.second.isNull();
    }
    TEST_TRUE(present)
    TEST_TRUE(empty)
  }
END_SECTION

// writes MzTabFile_SILAC.mzTab to filename, with extra_lines inserted after its first metadata line
auto storeSILACWithMetaData = [](const std::string& filename, const std::vector<std::string>& extra_lines)
{
  TextFile text(OPENMS_GET_TEST_DATA_PATH("MzTabFile_SILAC.mzTab"));
  TextFile modified;
  bool inserted = false;
  for (const auto& line : text)
  {
    modified.addLine(line);
    if (!inserted && StringUtils::hasPrefix(line, "MTD\t"))
    {
      for (const auto& extra : extra_lines) modified.addLine(extra);
      inserted = true;
    }
  }
  modified.store(filename);
};

START_SECTION(([EXTRA] metadata lines with an empty or incomplete key are rejected))
{
  MzTab mz_tab;

  std::string empty_key;
  NEW_TMP_FILE(empty_key)
  storeSILACWithMetaData(empty_key, {"MTD\t\tx"});
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, MzTabFile().load(empty_key, mz_tab),
    "Error parsing MzTab line: MTD\t\tx. The metadata key is empty in: " + empty_key)

  // an indexed key that lacks the "-" separated field its family requires
  std::string no_field;
  NEW_TMP_FILE(no_field)
  storeSILACWithMetaData(no_field, {"MTD\tinstrument[1]\tx"});
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, MzTabFile().load(no_field, mz_tab),
    "Error parsing MzTab metadata key 'instrument[1]': a '-' separated field of the key is missing in: " + no_field)

  // the unmodified file still loads
  std::string unmodified;
  NEW_TMP_FILE(unmodified)
  storeSILACWithMetaData(unmodified, {});
  MzTabFile().load(unmodified, mz_tab);
  TEST_NOT_EQUAL(mz_tab.getPSMSectionRows().size(), 0)
}
END_SECTION

START_SECTION(([EXTRA] column unit metadata is loaded and stored))
{
  const std::string protein_unit = "best_search_engine_score[1]=[UO, UO:0000186, dimensionless unit, ]";
  const std::string peptide_unit = "retention_time=[UO, UO:0000031, minute, ]";
  const std::string psm_unit = "retention_time=[UO, UO:0000010, second, ]";

  std::string filename;
  NEW_TMP_FILE(filename)
  storeSILACWithMetaData(filename, {"MTD\tcolunit-protein\t" + protein_unit,
                                    "MTD\tcolunit-peptide\t" + peptide_unit,
                                    "MTD\tcolunit-psm\t" + psm_unit});
  MzTab loaded;
  MzTabFile().load(filename, loaded);
  {
    const MzTabMetaData& md = loaded.getMetaData();
    TEST_EQUAL(md.colunit_protein.size(), 1)
    TEST_EQUAL(md.colunit_peptide.size(), 1)
    TEST_EQUAL(md.colunit_psm.size(), 1)
    TEST_EQUAL(md.colunit_small_molecule.size(), 0)
    ABORT_IF(md.colunit_protein.size() != 1 || md.colunit_peptide.size() != 1 || md.colunit_psm.size() != 1)
    TEST_EQUAL(md.colunit_protein[0], protein_unit)
    TEST_EQUAL(md.colunit_peptide[0], peptide_unit)
    TEST_EQUAL(md.colunit_psm[0], psm_unit)
  }

  // store: key and value are separate cells
  std::string stored;
  NEW_TMP_FILE(stored)
  MzTabFile().store(stored, loaded);
  TextFile stored_text(stored, true);
  TEST_EQUAL(std::count(stored_text.begin(), stored_text.end(), "MTD\tcolunit-protein\t" + protein_unit), 1)
  TEST_EQUAL(std::count(stored_text.begin(), stored_text.end(), "MTD\tcolunit-peptide\t" + peptide_unit), 1)
  TEST_EQUAL(std::count(stored_text.begin(), stored_text.end(), "MTD\tcolunit-PSM\t" + psm_unit), 1)

  // and the stored file loads back to the same units
  MzTab reloaded;
  MzTabFile().load(stored, reloaded);
  {
    const MzTabMetaData& md = reloaded.getMetaData();
    TEST_EQUAL(md.colunit_protein.size(), 1)
    TEST_EQUAL(md.colunit_peptide.size(), 1)
    TEST_EQUAL(md.colunit_psm.size(), 1)
    TEST_EQUAL(md.colunit_small_molecule.size(), 0)
    ABORT_IF(md.colunit_protein.size() != 1 || md.colunit_peptide.size() != 1 || md.colunit_psm.size() != 1)
    TEST_EQUAL(md.colunit_protein[0], protein_unit)
    TEST_EQUAL(md.colunit_peptide[0], peptide_unit)
    TEST_EQUAL(md.colunit_psm[0], psm_unit)
  }
}
END_SECTION

END_TEST
