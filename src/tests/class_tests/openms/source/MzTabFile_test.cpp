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
#include <OpenMS/DATASTRUCTURES/ListUtils.h>
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

START_SECTION(([EXTRA] a metadata key that is empty or has the field names but not the indices of an mzTab 1.0 key is rejected))
{
  for (const std::string& key : {std::string(), std::string(" ")})
  {
    std::string filename;
    NEW_TMP_FILE(filename)
    storeSILACWithMetaData(filename, {"MTD\t" + key + "\tx"});
    MzTab mz_tab;
    TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, MzTabFile().load(filename, mz_tab),
      "Error parsing MzTab line: MTD\t" + key + "\tx. The metadata key is empty in: " + filename)
  }

  const std::string empty_field = "a '-' separated field of the key is empty";
  const std::vector<std::pair<std::string, std::string>> malformed_keys =
  {
    {"-", empty_field},
    {"instrument[1]-", empty_field},
    {"instrument-name", "the mzTab 1.0 key with these field names has the form 'instrument[1-n]-name'"},
    {"instrument[x]-name", "the mzTab 1.0 key with these field names has the form 'instrument[1-n]-name'"},
    {"instrument[1-name", "the mzTab 1.0 key with these field names has the form 'instrument[1-n]-name'"},
    {"sample[1]-species", "the mzTab 1.0 key with these field names has the form 'sample[1-n]-species[1-n]'"},
    {"sample_processing[x]", "the mzTab 1.0 key with these field names has the form 'sample_processing[1-n]'"},
    {"ms_run[99999999999]-location", "the mzTab 1.0 key with these field names has the form 'ms_run[1-n]-location'"},
    {"colunit[3]-protein", "the mzTab 1.0 key with these field names has the form 'colunit-protein'"},
    {"protein[1]-quantification_unit", "the mzTab 1.0 key with these field names has the form 'protein-quantification_unit'"},
    {"title[1]", "the mzTab 1.0 key with these field names has the form 'title'"}
  };
  for (const auto& malformed : malformed_keys)
  {
    std::string filename;
    NEW_TMP_FILE(filename)
    storeSILACWithMetaData(filename, {"MTD\t" + malformed.first + "\tx"});
    MzTab mz_tab;
    TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, MzTabFile().load(filename, mz_tab),
      "Error parsing MzTab metadata key '" + malformed.first + "': " + malformed.second + " in: " + filename)
  }

  // the unmodified file still loads completely
  std::string unmodified;
  NEW_TMP_FILE(unmodified)
  storeSILACWithMetaData(unmodified, {});
  MzTab mz_tab;
  MzTabFile().load(unmodified, mz_tab);
  TEST_EQUAL(mz_tab.getProteinSectionRows().size(), 57)
  TEST_EQUAL(mz_tab.getPeptideSectionRows().size(), 80)
  TEST_EQUAL(mz_tab.getPSMSectionRows().size(), 946)
  TEST_EQUAL(mz_tab.getMetaData().instrument.size(), 1)
  TEST_EQUAL(mz_tab.getMetaData().assay.size(), 12)
}
END_SECTION

START_SECTION(([EXTRA] a metadata key that is not an mzTab 1.0 key is ignored))
{
  // keys of mzTab-M, and keys that lack a field of the mzTab 1.0 key they resemble
  const std::vector<std::string> keys = {"assay[1]", "study_variable[1]", "sample[1]", "database[1]-prefix", "cv[1]-uri",
                                         "ms_run[1]-scan_polarity[1]", "colunit-small_molecule_feature",
                                         "instrument[1]", "contact[1]", "ms_run[1]", "protein", "colunit"};
  std::vector<std::string> lines;
  for (const std::string& key : keys) lines.push_back("MTD\t" + key + "\tx");

  std::string with_keys;
  NEW_TMP_FILE(with_keys)
  storeSILACWithMetaData(with_keys, lines);
  MzTab loaded;
  MzTabFile().load(with_keys, loaded);
  TEST_EQUAL(loaded.getProteinSectionRows().size(), 57)
  TEST_EQUAL(loaded.getPeptideSectionRows().size(), 80)
  TEST_EQUAL(loaded.getPSMSectionRows().size(), 946)

  // nothing of these lines is kept: the metadata stores to the same lines as without them (only the
  // metadata lines are compared, since empty and comment lines are stored at their original line numbers)
  std::string without_keys;
  NEW_TMP_FILE(without_keys)
  storeSILACWithMetaData(without_keys, {});
  MzTab loaded_without_keys;
  MzTabFile().load(without_keys, loaded_without_keys);
  auto storedMetaData = [](const MzTab& mz_tab)
  {
    std::string stored;
    NEW_TMP_FILE(stored)
    MzTabFile().store(stored, mz_tab);
    std::vector<std::string> metadata;
    for (const auto& line : TextFile(stored, true))
    {
      if (StringUtils::hasPrefix(line, "MTD\t")) metadata.push_back(line);
    }
    return metadata;
  };
  const std::vector<std::string> metadata_without_keys = storedMetaData(loaded_without_keys);
  TEST_NOT_EQUAL(metadata_without_keys.size(), 0)
  TEST_EQUAL(ListUtils::concatenate(storedMetaData(loaded), "\n"), ListUtils::concatenate(metadata_without_keys, "\n"))
}
END_SECTION

START_SECTION(([EXTRA] the metadata of mzTab-M files is not rejected))
{
  for (const std::string& name : {std::string("MzTabMFile_output_1.mztab"),
                                  std::string("AccurateMassSearchEngine_output1_mztabm_featureXML.mzTab"),
                                  std::string("AccurateMassSearchEngine_output2_mztabm_featureXML.mzTab")})
  {
    // the metadata section alone loads, and its mzTab 1.0 keys are read
    TextFile text(OPENMS_GET_TEST_DATA_PATH(name));
    TextFile metadata;
    for (const auto& line : text)
    {
      if (StringUtils::hasPrefix(line, "MTD\t")) metadata.addLine(line);
    }
    std::string filename;
    NEW_TMP_FILE(filename)
    metadata.store(filename);
    MzTab mz_tab;
    MzTabFile().load(filename, mz_tab);
    const MzTabMetaData& md = mz_tab.getMetaData();
    TEST_EQUAL(md.mz_tab_version.get(), "2.0.0-M")
    TEST_EQUAL(md.ms_run.size(), 1)
    TEST_EQUAL(md.assay.size(), 1) // from assay[1]-ms_run_ref, not from the mzTab-M key assay[1]
    TEST_EQUAL(md.study_variable.size(), 1)
    TEST_EQUAL(md.cv.size(), 1)

    // the whole file does not fail at its metadata either (MzTabFile does not read the mzTab-M
    // small molecule section, which ends the load with another exception)
    bool parse_error = false;
    try
    {
      MzTab whole_file;
      MzTabFile().load(OPENMS_GET_TEST_DATA_PATH(name), whole_file);
    }
    catch (const Exception::ParseError&)
    {
      parse_error = true;
    }
    catch (const Exception::BaseException&)
    {
    }
    TEST_FALSE(parse_error)
  }
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

  // store: key and value are separate cells. The key is compared case-insensitively, as the reader
  // compares it, so both the specification's "colunit-psm" and the writer's "colunit-PSM" match.
  std::string stored;
  NEW_TMP_FILE(stored)
  MzTabFile().store(stored, loaded);
  TextFile stored_text(stored, true);
  auto countMetaData = [&stored_text](const std::string& key, const std::string& value)
  {
    return std::count_if(stored_text.begin(), stored_text.end(), [&](const std::string& line)
    {
      std::vector<std::string> cells;
      StringUtils::split(line, '\t', cells);
      return cells.size() == 3 && cells[0] == "MTD" && StringUtils::toLowered(cells[1]) == key && cells[2] == value;
    });
  };
  TEST_EQUAL(countMetaData("colunit-protein", protein_unit), 1)
  TEST_EQUAL(countMetaData("colunit-peptide", peptide_unit), 1)
  TEST_EQUAL(countMetaData("colunit-psm", psm_unit), 1)

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
