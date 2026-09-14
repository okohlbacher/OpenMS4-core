// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>

///////////////////////////
#include <OpenMS/FORMAT/HANDLERS/MzMLSqliteHandler.h>
///////////////////////////

#include <OpenMS/FORMAT/MzMLFile.h>
#include <OpenMS/FORMAT/SqliteConnector.h>
#include <OpenMS/KERNEL/MSExperiment.h>

#include <filesystem>

using namespace OpenMS;
using namespace OpenMS::Internal;
using namespace std;

void cmpDataIntensity(const MSExperiment& exp1, const MSExperiment& exp2, double abs_tol = 1e-5, double rel_tol = 1+1e-5)
{
  // Logic of comparison: if the absolute difference criterion is fulfilled,
  // the relative one does not matter. If the absolute difference is larger
  // than allowed, the test does not fail if the relative difference is less
  // than allowed.
  // Note that the sample spectrum intensity has a very large range, from
  // 0.00013 to 183 838 intensity and encoding both values with high accuracy
  // is difficult.

  TOLERANCE_ABSOLUTE(abs_tol)
  TOLERANCE_RELATIVE(rel_tol)
  for (Size i = 0; i < exp1.getNrSpectra(); i++)
  {
    TEST_EQUAL(exp1.getSpectra()[i].size(), exp2.getSpectra()[i].size())
    for (Size k = 0; k < exp1.getSpectra()[i].size(); k++)
    {
      // slof is no good for values smaller than 5
      // if (exp.getSpectrum(i)[k].getIntensity() < 1.0) {continue;} 
      auto a = exp1.getSpectra()[i][k].getIntensity();
      auto b = exp2.getSpectra()[i][k].getIntensity();
      // avoid the console clutter if nothing interesing happens
      if (!TEST::isRealSimilar(a, b)) TEST_REAL_SIMILAR(a, b)
    }
  }

  for (Size i = 0; i < exp1.getNrChromatograms(); i++)
  {
    TEST_EQUAL(exp1.getChromatograms()[i].size() == exp2.getChromatograms()[i].size(), true)
    for (Size k = 0; k < exp1.getChromatograms()[i].size(); k++)
    {
      auto a = exp1.getChromatograms()[i][k].getIntensity();
      auto b = exp2.getChromatograms()[i][k].getIntensity();
      // avoid the console clutter if nothing interesing happens
      if (!TEST::isRealSimilar(a, b)) TEST_REAL_SIMILAR(a, b)
    }
  }
}

void cmpDataMZ(const MSExperiment& exp1, const MSExperiment& exp2, double abs_tol = 1e-5, double rel_tol = 1+1e-5)
{
  // Logic of comparison: if the absolute difference criterion is fulfilled,
  // the relative one does not matter. If the absolute difference is larger
  // than allowed, the test does not fail if the relative difference is less
  // than allowed.
  // Note that the sample spectrum intensity has a very large range, from
  // 0.00013 to 183 838 intensity and encoding both values with high accuracy
  // is difficult.

  TOLERANCE_ABSOLUTE(abs_tol)
  TOLERANCE_RELATIVE(rel_tol)
  for (Size i = 0; i < exp1.getNrSpectra(); i++)
  {
    TEST_EQUAL(exp1.getSpectra()[i].size(), exp2.getSpectra()[i].size())
    for (Size k = 0; k < exp1.getSpectra()[i].size(); k++)
    {
      // slof is no good for values smaller than 5
      // if (exp.getSpectrum(i)[k].getIntensity() < 1.0) {continue;} 
      auto a = exp1.getSpectra()[i][k].getMZ();
      auto b = exp2.getSpectra()[i][k].getMZ();
      // avoid the console clutter if nothing interesing happens
      if (!TEST::isRealSimilar(a, b)) TEST_REAL_SIMILAR(a, b)
    }
  }
}

void cmpDataRT(const MSExperiment& exp1, const MSExperiment& exp2, double abs_tol = 1e-5, double rel_tol = 1+1e-5)
{
  // Logic of comparison: if the absolute difference criterion is fulfilled,
  // the relative one does not matter. If the absolute difference is larger
  // than allowed, the test does not fail if the relative difference is less
  // than allowed.
  // Note that the sample spectrum intensity has a very large range, from
  // 0.00013 to 183 838 intensity and encoding both values with high accuracy
  // is difficult.

  TOLERANCE_ABSOLUTE(abs_tol)
  TOLERANCE_RELATIVE(rel_tol)
  for (Size i = 0; i < exp1.getNrChromatograms(); i++)
  {
    TEST_EQUAL(exp1.getChromatograms()[i].size() == exp2.getChromatograms()[i].size(), true)
    for (Size k = 0; k < exp1.getChromatograms()[i].size(); k++)
    {
      auto a = exp1.getChromatograms()[i][k].getRT();
      auto b = exp2.getChromatograms()[i][k].getRT();
      // avoid the console clutter if nothing interesing happens
      if (!TEST::isRealSimilar(a, b)) TEST_REAL_SIMILAR(a, b)
    }
  }
}

///////////////////////////

START_TEST(MzMLSqliteHandler, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

MzMLSqliteHandler* ptr = nullptr;
MzMLSqliteHandler* nullPointer = nullptr;
START_SECTION((MzMLSqliteHandler(const std::string& filename, const UInt64 run_id)))
  ptr = new MzMLSqliteHandler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);
  TEST_NOT_EQUAL(ptr, nullPointer)
END_SECTION

START_SECTION((~MzMLSqliteHandler()))
  delete ptr;
END_SECTION

TOLERANCE_RELATIVE(1.0005)

START_SECTION(UInt64 getRunID() const)
  MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);
  TEST_EQUAL(handler.getRunID(), 12345)
END_SECTION

START_SECTION(void readExperiment(MSExperiment & exp, bool meta_only = false) const)
{
  MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  MSExperiment exp_orig;
  MzMLFile().load(OPENMS_GET_TEST_DATA_PATH("MzMLSqliteHandler_1.mzML"), exp_orig);

  // read in meta data only
  {
    MSExperiment exp;
    handler.readExperiment(exp, true);
    TEST_EQUAL(exp.getNrSpectra(), exp_orig.getSpectra().size())
    TEST_EQUAL(exp.getNrChromatograms(), exp_orig.getChromatograms().size())
    TEST_EQUAL(exp.getNrSpectra(), 2)
    TEST_EQUAL(exp.getNrChromatograms(), 1)
    TEST_EQUAL(exp.getSpectrum(0) == exp_orig.getSpectra()[0], false) // no exact duplicate

    for (Size i = 0; i < exp.getNrSpectra(); i++)
    {
      TEST_EQUAL(exp.getSpectrum(i).size(), 0)
    }

    for (Size i = 0; i < exp.getNrChromatograms(); i++)
    {
      TEST_EQUAL(exp.getChromatogram(i).size(), 0)
    }
    TEST_EQUAL(exp.getExperimentalSettings() == (OpenMS::ExperimentalSettings)exp_orig, true)
    TEST_EQUAL(exp.getSqlRunID(), 12345)
  } 
  // read in all data
  {
    MSExperiment exp;
    handler.readExperiment(exp, false);

    TEST_EQUAL(exp.getNrSpectra(), exp_orig.getSpectra().size())
    TEST_EQUAL(exp.getNrChromatograms(), exp_orig.getChromatograms().size())
    TEST_EQUAL(exp.getNrSpectra(), 2)
    TEST_EQUAL(exp.getNrChromatograms(), 1)
    TEST_EQUAL(exp.getSpectrum(0) == exp_orig.getSpectra()[0], false) // no exact duplicate

    cout.precision(17);

    cmpDataIntensity(exp, exp_orig, 1e-4, 1.001); 
    cmpDataMZ(exp, exp_orig, 1e-5, 1.000001); // less than 1ppm error for m/z 
    cmpDataRT(exp, exp_orig, 0.05, 1.000001); // max 0.05 seconds error in RT

    // 1:1 mapping of experimental settings ...
    TEST_EQUAL(exp.getExperimentalSettings() == (OpenMS::ExperimentalSettings)exp_orig, true)
    TEST_EQUAL(exp.getSqlRunID(), 12345)
  }
}
END_SECTION

START_SECTION( Size getNrSpectra() const )
{
  MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);
  TEST_EQUAL(handler.getNrSpectra(), 2)
}
END_SECTION

START_SECTION( Size getNrChromatograms() const )
{
  MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);
  TEST_EQUAL(handler.getNrChromatograms(), 1)
}
END_SECTION

START_SECTION( void readSpectra(std::vector<MSSpectrum> & exp, const std::vector<int> & indices, bool meta_only = false) const)
{
  MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  MSExperiment exp2;
  MzMLFile().load(OPENMS_GET_TEST_DATA_PATH("MzMLSqliteHandler_1.mzML"), exp2);

  // read in meta data only
  {
    std::vector<MSSpectrum> exp;
    std::vector<int> indices = {1};
    handler.readSpectra(exp, indices, true);
    TEST_EQUAL(exp.size(), 1)
    TEST_EQUAL(exp[0].size(), 0)
    TEST_REAL_SIMILAR(exp[0].getRT(), 0.4738)
  }

  {
    std::vector<MSSpectrum> exp;
    std::vector<int> indices = {1};
    handler.readSpectra(exp, indices, false);
    TEST_EQUAL(exp.size(), 1)
    TEST_EQUAL(exp[0].size(), 19800)
    TEST_REAL_SIMILAR(exp[0].getRT(), 0.4738)
  }

  {
    std::vector<MSSpectrum> exp;
    std::vector<int> indices = {0};
    handler.readSpectra(exp, indices, false);
    TEST_EQUAL(exp.size(), 1)
    TEST_EQUAL(exp[0].size(), 19914)
    TEST_REAL_SIMILAR(exp[0].getRT(), 0.2961)
  }

  {
    std::vector<MSSpectrum> exp;
    std::vector<int> indices = {0, 1};
    handler.readSpectra(exp, indices, true);
    TEST_EQUAL(exp.size(), 2)
    TEST_EQUAL(exp[0].size(), 0)
    TEST_EQUAL(exp[1].size(), 0)
    TEST_REAL_SIMILAR(exp[0].getRT(), 0.2961)
    TEST_REAL_SIMILAR(exp[1].getRT(), 0.4738)
  }

  {
    std::vector<MSSpectrum> exp;
    std::vector<int> indices = {0, 1};
    handler.readSpectra(exp, indices, false);
    TEST_EQUAL(exp.size(), 2)
    TEST_EQUAL(exp[0].size(), 19914)
    TEST_EQUAL(exp[1].size(), 19800)
    TEST_REAL_SIMILAR(exp[0].getRT(), 0.2961)
    TEST_REAL_SIMILAR(exp[1].getRT(), 0.4738)
  }

  {
    std::vector<MSSpectrum> exp;
    std::vector<int> indices = {0, 1, 2};
    TEST_EXCEPTION(Exception::IllegalArgument, handler.readSpectra(exp, indices, false));
  }

  {
    std::vector<MSSpectrum> exp;
    std::vector<int> indices = {5};
    TEST_EXCEPTION(Exception::IllegalArgument, handler.readSpectra(exp, indices, false));
  }
}
END_SECTION

START_SECTION(void readChromatograms(std::vector<MSChromatogram> & exp, const std::vector<int> & indices, bool meta_only = false) const)
{
  MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  MSExperiment exp2;
  MzMLFile().load(OPENMS_GET_TEST_DATA_PATH("MzMLSqliteHandler_1.mzML"), exp2);

  // read in meta data only
  {
    std::vector<MSChromatogram> exp;
    std::vector<int> indices = {0};
    handler.readChromatograms(exp, indices, true);
    TEST_EQUAL(exp.size(), 1)
    TEST_EQUAL(exp[0].size(), 0)
    TEST_STRING_EQUAL(exp[0].getNativeID(), "TIC")
  }

  {
    std::vector<MSChromatogram> exp;
    std::vector<int> indices = {0, 1};
    TEST_EXCEPTION(Exception::IllegalArgument, handler.readChromatograms(exp, indices, false));
  }

  {
    std::vector<MSChromatogram> exp;
    std::vector<int> indices = {5};
    TEST_EXCEPTION(Exception::IllegalArgument, handler.readChromatograms(exp, indices, false));
  }

  {

    MSExperiment exp_orig;
    MzMLFile().load(OPENMS_GET_TEST_DATA_PATH("MzMLSqliteHandler_1.mzML"), exp_orig);

    std::string tmp_filename;
    NEW_TMP_FILE(tmp_filename);

    // delete file if present
    std::filesystem::remove(tmp_filename);

    auto chroms = exp_orig.getChromatograms();
    chroms.push_back(exp_orig.getChromatograms()[0]);
    chroms.back().setNativeID("second");

    {
      MzMLSqliteHandler handler(tmp_filename, 0);
      handler.setConfig(true, false, 0.0001);
      handler.createTables();
      handler.writeChromatograms(chroms);
    }

    MzMLSqliteHandler handler(tmp_filename, 0);
    {
      std::vector<MSChromatogram> exp;
      std::vector<int> indices = {0};
      handler.readChromatograms(exp, indices, true);
      TEST_EQUAL(exp.size(), 1)
      TEST_EQUAL(exp[0].size(), 0)
      TEST_STRING_EQUAL(exp[0].getNativeID(), "TIC")
    }

    {
      std::vector<MSChromatogram> exp;
      std::vector<int> indices = {1};
      handler.readChromatograms(exp, indices, true);
      TEST_EQUAL(exp.size(), 1)
      TEST_EQUAL(exp[0].size(), 0)
      TEST_STRING_EQUAL(exp[0].getNativeID(), "second")
    }

    {
      std::vector<MSChromatogram> exp;
      std::vector<int> indices = {0, 1};
      handler.readChromatograms(exp, indices, true);
      TEST_EQUAL(exp.size(), 2)
      TEST_EQUAL(exp[0].size(), 0)
      TEST_STRING_EQUAL(exp[0].getNativeID(), "TIC")
      TEST_STRING_EQUAL(exp[1].getNativeID(), "second")
    }

  }

}
END_SECTION

START_SECTION(std::vector<size_t> getSpectraIndicesbyRT(double RT, double deltaRT, const std::vector<int> & indices) const)
{
  MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  {
    std::vector<int> indices = {};
    auto res = handler.getSpectraIndicesbyRT(0.4738, 0.1, indices);
    TEST_EQUAL(res.size(), 1)
    TEST_EQUAL(res[0], 1)
  }

  {
    std::vector<int> indices = {};
    auto res = handler.getSpectraIndicesbyRT(0.296, 0.1, indices);
    TEST_EQUAL(res.size(), 1)
    TEST_EQUAL(res[0], 0)
  }

  {
    std::vector<int> indices = {};
    auto res = handler.getSpectraIndicesbyRT(0.296, 1.1, indices);
    TEST_EQUAL(res.size(), 2)
    TEST_EQUAL(res[0], 0)
    TEST_EQUAL(res[1], 1)
  }

  {
    std::vector<int> indices = {1};
    auto res = handler.getSpectraIndicesbyRT(0.296, 1.1, indices);
    TEST_EQUAL(res.size(), 1)
    TEST_EQUAL(res[0], 1)
  }

  {
    std::vector<int> indices = {0};
    auto res = handler.getSpectraIndicesbyRT(0.296, 1.1, indices);
    TEST_EQUAL(res.size(), 1)
    TEST_EQUAL(res[0], 0)
  }

  {
    std::vector<int> indices = {};
    auto res = handler.getSpectraIndicesbyRT(0.0, 0.1, indices);
    TEST_EQUAL(res.size(), 0)
  }

  // negative deltaRT will simply return the first spectrum
  {
    std::vector<int> indices = {};
    auto res = handler.getSpectraIndicesbyRT(0.3, -0.1, indices);
    TEST_EQUAL(res.size(), 1)
    TEST_EQUAL(res[0], 1)
  }

  {
    std::vector<int> indices = {};
    auto res = handler.getSpectraIndicesbyRT(0.0, -0.1, indices);
    TEST_EQUAL(res.size(), 1)
    TEST_EQUAL(res[0], 0)
  }


}
END_SECTION

START_SECTION(void writeExperiment(const MSExperiment & exp))
{
  const MSExperiment exp_orig = [](){
    MSExperiment tmp;
    MzMLFile().load(OPENMS_GET_TEST_DATA_PATH("MzMLSqliteHandler_1.mzML"), tmp);
    return tmp;
  }();

  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);

  // delete file if present
  std::filesystem::remove(tmp_filename);

  {
    MzMLSqliteHandler handler(tmp_filename, 12345);
    // writing without creating the tables / indices won't work
    TEST_EXCEPTION(Exception::IllegalArgument, handler.writeExperiment(exp_orig));

    // now it will work
    handler.createTables();
    handler.createTables();
    handler.writeExperiment(exp_orig);

    // you can createTables() twice, but it will delete all your data 
    TEST_EQUAL(handler.getNrSpectra(), 2)
    handler.createTables();
    TEST_EQUAL(handler.getNrSpectra(), 0)
    handler.writeExperiment(exp_orig);
    TEST_EQUAL(handler.getNrSpectra(), 2)
  }

  MzMLSqliteHandler handler(tmp_filename, 12345);
  // read in meta data only
  {
    MSExperiment exp;
    handler.readExperiment(exp, true);
    TEST_EQUAL(exp.getNrSpectra(), exp_orig.getSpectra().size())
    TEST_EQUAL(exp.getNrChromatograms(), exp_orig.getChromatograms().size())
    TEST_EQUAL(exp.getNrSpectra(), 2)
    TEST_EQUAL(exp.getNrChromatograms(), 1)
    TEST_EQUAL(exp.getSpectrum(0) == exp_orig.getSpectra()[0], false) // no exact duplicate

    for (Size i = 0; i < exp.getNrSpectra(); i++)
    {
      TEST_EQUAL(exp.getSpectrum(i).size(), 0)
    }

    for (Size i = 0; i < exp.getNrChromatograms(); i++)
    {
      TEST_EQUAL(exp.getChromatogram(i).size(), 0)
    }
    TEST_EQUAL(exp.getExperimentalSettings() == (OpenMS::ExperimentalSettings)exp_orig, true)
  }

  MSExperiment exp;
  handler.readExperiment(exp, false);
  // tmp:
  //MzMLFile().store(OPENMS_GET_TEST_DATA_PATH("MzMLSqliteHandler_1.mzML"), exp);

  TEST_EQUAL(exp.getNrSpectra(), exp_orig.getSpectra().size())
  TEST_EQUAL(exp.getNrChromatograms(), exp_orig.getChromatograms().size())
  TEST_EQUAL(exp.getNrSpectra(), 2)
  TEST_EQUAL(exp.getNrChromatograms(), 1)
  TEST_EQUAL(exp.getSpectrum(0) == exp_orig.getSpectra()[0], false) // no exact duplicate

  cmpDataIntensity(exp, exp_orig, 1e-4, 1.001);
  cmpDataMZ(exp, exp_orig, 1e-5, 1.000001); // less than 1ppm error for m/z 
  cmpDataRT(exp, exp_orig, 0.05, 1.000001); // max 0.05 seconds error in RT

  // 1:1 mapping of experimental settings ...
  TEST_EQUAL(exp.getExperimentalSettings() == (OpenMS::ExperimentalSettings)exp_orig, true)
}
END_SECTION

START_SECTION([EXTRA] writeRunLevelInformation is safe against SQL injection via the loaded file path)
{
  // Regression test for GH #9730: the loaded file path was concatenated into an
  // INSERT executed via sqlite3_exec, so a path containing a single quote broke
  // the statement and a crafted path could inject arbitrary SQL.
  MSExperiment exp_orig;
  MzMLFile().load(OPENMS_GET_TEST_DATA_PATH("MzMLSqliteHandler_1.mzML"), exp_orig);

  // Leading '/' => setLoadedFilePath keeps it verbatim (treated absolute on all
  // platforms) instead of prepending the CWD.
  const std::string evil_path = "/tmp/o'brien'; DROP TABLE RUN;--/run.mzML";
  exp_orig.setLoadedFilePath(evil_path);
  TEST_EQUAL(exp_orig.getLoadedFilePath(), evil_path) // sanity: stored verbatim

  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);
  std::filesystem::remove(tmp_filename);

  MzMLSqliteHandler handler(tmp_filename, 12345);
  handler.createTables();
  // With the old concatenating code this throws (malformed SQL) or runs the
  // injected DROP; with the parameterized fix it must simply succeed.
  handler.writeExperiment(exp_orig);

  // The RUN table must still exist and hold exactly the one row we wrote: if the
  // injected "DROP TABLE RUN" had executed, countTableRows() would instead throw.
  SqliteConnector conn(tmp_filename);
  TEST_EQUAL(conn.countTableRows("RUN"), 1)
}
END_SECTION

START_SECTION(void writeSpectra(const std::vector<MSSpectrum>& spectra))
{
  MSExperiment exp_orig;
  MzMLFile().load(OPENMS_GET_TEST_DATA_PATH("MzMLSqliteHandler_1.mzML"), exp_orig);

  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);

  // delete file if present
  std::filesystem::remove(tmp_filename);

  {
    MzMLSqliteHandler handler(tmp_filename, 12345);
    // writing without creating the tables / indices won't work
    TEST_EXCEPTION(Exception::IllegalArgument, handler.writeSpectra(exp_orig.getSpectra()));

    // now it will work
    handler.createTables();
    handler.createTables();
    handler.writeSpectra(exp_orig.getSpectra());
    TEST_EQUAL(handler.getNrSpectra(), 2)
    handler.writeSpectra(exp_orig.getSpectra());
    TEST_EQUAL(handler.getNrSpectra(), 4)
    handler.writeSpectra(exp_orig.getSpectra());
    TEST_EQUAL(handler.getNrSpectra(), 6)
    handler.writeRunLevelInformation(exp_orig, false);
    MSExperiment tmp;
    handler.readExperiment(tmp, false);
    TEST_EQUAL(tmp.getNrSpectra(), 6)
    TEST_EQUAL(tmp[0].size(), 19914)
    TEST_EQUAL(tmp[1].size(), 19800)
    TEST_EQUAL(tmp[2].size(), 19914)
    TEST_EQUAL(tmp[3].size(), 19800)
    TEST_EQUAL(tmp[4].size(), 19914)
    TEST_EQUAL(tmp[5].size(), 19800)

    TEST_REAL_SIMILAR(tmp.getSpectra()[0][100].getMZ(), 204.817)
    TEST_REAL_SIMILAR(tmp.getSpectra()[0][100].getIntensity(), 3857.86)

    // clear
    handler.createTables();
    handler.writeSpectra(exp_orig.getSpectra());
    TEST_EQUAL(handler.getNrSpectra(), 2)
  }

}
END_SECTION

START_SECTION(void writeChromatograms(const std::vector<MSChromatogram>& chroms))
{
  MSExperiment exp_orig;
  MzMLFile().load(OPENMS_GET_TEST_DATA_PATH("MzMLSqliteHandler_1.mzML"), exp_orig);

  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);

  // delete file if present
  std::filesystem::remove(tmp_filename);

  {
    MzMLSqliteHandler handler(tmp_filename, 12345);
    handler.setConfig(true, false, 0.0001);
    // writing without creating the tables / indices won't work
    TEST_EXCEPTION(Exception::IllegalArgument, handler.writeChromatograms(exp_orig.getChromatograms()));

    // now it will work
    handler.createTables();
    handler.createTables();
    handler.writeChromatograms(exp_orig.getChromatograms());
    TEST_EQUAL(handler.getNrChromatograms(), 1)
    handler.writeChromatograms(exp_orig.getChromatograms());
    TEST_EQUAL(handler.getNrChromatograms(), 2)
    handler.writeChromatograms(exp_orig.getChromatograms());
    TEST_EQUAL(handler.getNrChromatograms(), 3)
    handler.writeRunLevelInformation(exp_orig, false);

    MSExperiment tmp;
    handler.readExperiment(tmp, false);
    TEST_EQUAL(tmp.getNrChromatograms(), 3)
    TEST_EQUAL(tmp.getChromatograms()[0].size(), 48)
    TEST_EQUAL(tmp.getChromatograms()[1].size(), 48)
    TEST_EQUAL(tmp.getChromatograms()[2].size(), 48)

    TEST_REAL_SIMILAR(tmp.getChromatograms()[0][20].getRT(), 0.200695)
    TEST_REAL_SIMILAR(tmp.getChromatograms()[0][20].getIntensity(), 147414.578125)

    // clear
    handler.createTables();
    handler.writeChromatograms(exp_orig.getChromatograms());
    TEST_EQUAL(handler.getNrChromatograms(), 1)
  }

  // now test with numpress (accuracy is lower)
  TOLERANCE_RELATIVE(1+2e-4)
  // delete file if present
  std::filesystem::remove(tmp_filename);
  {
    MzMLSqliteHandler handler(tmp_filename, 12345);
    handler.setConfig(true, true, 0.0001);
    // writing without creating the tables / indices won't work
    TEST_EXCEPTION(Exception::IllegalArgument, handler.writeChromatograms(exp_orig.getChromatograms()));

    // now it will work
    handler.createTables();
    handler.createTables();
    handler.writeChromatograms(exp_orig.getChromatograms());
    TEST_EQUAL(handler.getNrChromatograms(), 1)
    handler.writeChromatograms(exp_orig.getChromatograms());
    TEST_EQUAL(handler.getNrChromatograms(), 2)
    handler.writeChromatograms(exp_orig.getChromatograms());
    TEST_EQUAL(handler.getNrChromatograms(), 3)
    handler.writeRunLevelInformation(exp_orig, false);

    MSExperiment tmp;
    handler.readExperiment(tmp, false);
    TEST_EQUAL(tmp.getNrChromatograms(), 3)
    TEST_EQUAL(tmp.getChromatograms()[0].size(), 48)
    TEST_EQUAL(tmp.getChromatograms()[1].size(), 48)
    TEST_EQUAL(tmp.getChromatograms()[2].size(), 48)

    TEST_REAL_SIMILAR(tmp.getChromatograms()[0][20].getRT(), 0.200695)
    TEST_REAL_SIMILAR(tmp.getChromatograms()[0][20].getIntensity(), 147414.578125)

    // clear
    handler.createTables();
    handler.writeChromatograms(exp_orig.getChromatograms());
    TEST_EQUAL(handler.getNrChromatograms(), 1)
  }
}
END_SECTION

START_SECTION([EXTRA] reading rejects data arrays of different length and duplicated array roles)
{
  // Each record was sized by its first data array (or by the next one, after an empty first array)
  // and every further array was copied by that length without a check (a shorter one was read past
  // its end, a longer one truncated), and the rows were only counted, so two m/z arrays passed as an
  // m/z and an intensity array. The malformed files are written with the API and their DATA table
  // altered afterwards.
  std::vector<MSSpectrum> spectra(3);
  for (Size i = 0; i < spectra.size(); ++i)
  {
    spectra[i].setNativeID("spectrum=" + std::to_string(i));
    spectra[i].setRT(10.0 * (i + 1));
  }
  spectra[0].push_back(Peak1D(100.0, 1000.0));
  spectra[0].push_back(Peak1D(200.0, 2000.0));
  spectra[0].push_back(Peak1D(300.0, 3000.0));
  spectra[1].push_back(Peak1D(400.0, 4000.0));
  spectra[1].push_back(Peak1D(500.0, 5000.0));
  // spectra[2] has no peaks

  std::vector<MSChromatogram> chroms(3);
  for (Size i = 0; i < chroms.size(); ++i)
  {
    chroms[i].setNativeID("chromatogram=" + std::to_string(i));
  }
  chroms[0].push_back(ChromatogramPeak(1.0, 10.0));
  chroms[0].push_back(ChromatogramPeak(2.0, 20.0));
  chroms[0].push_back(ChromatogramPeak(3.0, 30.0));
  chroms[1].push_back(ChromatogramPeak(4.0, 40.0));
  chroms[1].push_back(ChromatogramPeak(5.0, 50.0));
  // chroms[2] has no peaks

  for (bool lossy : {false, true})
  {
    const std::string extension = lossy ? ".lossy" : ".lossless";
    // writes the records above to 'filename', then applies 'alter_sql' to the file
    auto writeAltered = [&](const std::string& filename, const std::string& alter_sql)
    {
      std::filesystem::remove(filename);
      {
        MzMLSqliteHandler handler(filename, 0);
        handler.setConfig(false, lossy, 0.0001);
        handler.createTables();
        handler.writeSpectra(spectra);
        handler.writeChromatograms(chroms);
        handler.writeRunLevelInformation(MSExperiment(), false);
      }
      if (!alter_sql.empty())
      {
        SqliteConnector conn(filename);
        conn.executeStatement(alter_sql);
      }
    };

    // unaltered: every record loads, including the ones without peaks
    {
      STATUS((lossy ? "lossy" : "lossless") << ": unaltered")
      std::string filename;
      NEW_TMP_FILE_EXT(filename, extension);
      writeAltered(filename, "");
      MzMLSqliteHandler handler(filename, 0);
      std::vector<MSSpectrum> read_spectra;
      handler.readSpectra(read_spectra, {0, 1, 2});
      ABORT_IF(read_spectra.size() != 3)
      TEST_EQUAL(read_spectra[0].size(), 3)
      TEST_EQUAL(read_spectra[1].size(), 2)
      TEST_EQUAL(read_spectra[2].size(), 0)
      TEST_REAL_SIMILAR(read_spectra[0][2].getMZ(), 300.0)
      TEST_REAL_SIMILAR(read_spectra[0][2].getIntensity(), 3000.0)

      std::vector<MSChromatogram> read_chroms;
      handler.readChromatograms(read_chroms, {0, 1, 2});
      ABORT_IF(read_chroms.size() != 3)
      TEST_EQUAL(read_chroms[0].size(), 3)
      TEST_EQUAL(read_chroms[1].size(), 2)
      TEST_EQUAL(read_chroms[2].size(), 0)

      MSExperiment exp;
      handler.readExperiment(exp);
      TEST_EQUAL(exp.getNrSpectra(), 3)
      TEST_EQUAL(exp.getNrChromatograms(), 3)
    }

    // The rows of one record are read in the order SQLite returns them, which the query does not fix
    // (it orders by record only). Each length edit below is therefore written twice, once with the
    // intensity row and once with the m/z (RT) row re-inserted at the end of the table, so that each
    // of the two arrays is the one read second in one of the files.
    // 'replaceIntensities' gives record 1 (2 peaks) the intensity array of record 'donor' and moves
    // its row of 'last_type' to the end; 'id' is SPECTRUM_ID or CHROMATOGRAM_ID.
    auto replaceIntensities = [](const std::string& id, int donor, int last_type)
    {
      const std::string donor_row = " FROM DATA WHERE " + id + " = " + std::to_string(donor) + " AND DATA_TYPE = 1)";
      const std::string last_row = " FROM DATA WHERE " + id + " = 1 AND DATA_TYPE = " + std::to_string(last_type);
      return "UPDATE DATA SET COMPRESSION = (SELECT COMPRESSION" + donor_row + ", DATA = (SELECT DATA" + donor_row +
             " WHERE " + id + " = 1 AND DATA_TYPE = 1;"
             "INSERT INTO DATA (SPECTRUM_ID, CHROMATOGRAM_ID, COMPRESSION, DATA_TYPE, DATA)"
             " SELECT SPECTRUM_ID, CHROMATOGRAM_ID, COMPRESSION, DATA_TYPE, DATA" + last_row + ";"
             "DELETE FROM DATA WHERE rowid = (SELECT MIN(rowid)" + last_row + ");";
    };
    // record 1 with 3 intensities (record 0's, longer than its 2 m/z or RT values) or none (record 2's)
    for (int donor : {0, 2})
    {
      for (bool intensities_last : {true, false})
      {
        STATUS((lossy ? "lossy" : "lossless") << ": record 1 with the intensity array of record " << donor << ", "
               << (intensities_last ? "intensity" : "m/z (RT)") << " row last")
        // a file name per variant: a rejected read leaves its connection open (the file is locked on Windows)
        std::string filename;
        NEW_TMP_FILE_EXT(filename, ".donor" + std::to_string(donor) + (intensities_last ? ".intensity_last" : ".coordinate_last") + extension);
        writeAltered(filename, replaceIntensities("SPECTRUM_ID", donor, intensities_last ? 1 : 0) +
                               replaceIntensities("CHROMATOGRAM_ID", donor, intensities_last ? 1 : 2));
        MzMLSqliteHandler handler(filename, 0);
        std::vector<MSSpectrum> read_spectra;
        TEST_EXCEPTION(Exception::IllegalArgument, handler.readSpectra(read_spectra, {1}))
        read_spectra.clear();
        handler.readSpectra(read_spectra, {0, 2}); // not altered
        ABORT_IF(read_spectra.size() != 2)
        TEST_EQUAL(read_spectra[0].size(), 3)
        TEST_EQUAL(read_spectra[1].size(), 0)

        std::vector<MSChromatogram> read_chroms;
        TEST_EXCEPTION(Exception::IllegalArgument, handler.readChromatograms(read_chroms, {1}))
        read_chroms.clear();
        handler.readChromatograms(read_chroms, {0, 2}); // not altered
        ABORT_IF(read_chroms.size() != 2)
        TEST_EQUAL(read_chroms[0].size(), 3)
        TEST_EQUAL(read_chroms[1].size(), 0)

        MSExperiment exp;
        TEST_EXCEPTION(Exception::IllegalArgument, handler.readExperiment(exp))
      }
    }

    // record 0 with both arrays and a copy of one of them: the copy has the length of the others and
    // both roles are present, so only the check for a repeated role rejects it
    for (bool copy_intensities : {false, true})
    {
      STATUS((lossy ? "lossy" : "lossless") << ": record 0 with a second " << (copy_intensities ? "intensity" : "m/z (RT)") << " array")
      const std::string copy_row = "INSERT INTO DATA (SPECTRUM_ID, CHROMATOGRAM_ID, COMPRESSION, DATA_TYPE, DATA)"
                                   " SELECT SPECTRUM_ID, CHROMATOGRAM_ID, COMPRESSION, DATA_TYPE, DATA FROM DATA WHERE ";
      std::string filename;
      NEW_TMP_FILE_EXT(filename, (copy_intensities ? ".intensity_copy" : ".coordinate_copy") + extension);
      writeAltered(filename, copy_row + "SPECTRUM_ID = 0 AND DATA_TYPE = " + (copy_intensities ? "1" : "0") + ";" +
                             copy_row + "CHROMATOGRAM_ID = 0 AND DATA_TYPE = " + (copy_intensities ? "1" : "2") + ";");
      MzMLSqliteHandler handler(filename, 0);
      std::vector<MSSpectrum> read_spectra;
      TEST_EXCEPTION(Exception::IllegalArgument, handler.readSpectra(read_spectra, {0}))
      read_spectra.clear();
      handler.readSpectra(read_spectra, {1}); // not altered
      ABORT_IF(read_spectra.size() != 1)
      TEST_EQUAL(read_spectra[0].size(), 2)

      std::vector<MSChromatogram> read_chroms;
      TEST_EXCEPTION(Exception::IllegalArgument, handler.readChromatograms(read_chroms, {0}))
      read_chroms.clear();
      handler.readChromatograms(read_chroms, {1}); // not altered
      ABORT_IF(read_chroms.size() != 1)
      TEST_EQUAL(read_chroms[0].size(), 2)
    }

    // record 0 without an intensity array: only the check that every record has both roles rejects it
    {
      STATUS((lossy ? "lossy" : "lossless") << ": record 0 without an intensity array")
      std::string filename;
      NEW_TMP_FILE_EXT(filename, extension);
      writeAltered(filename,
          "DELETE FROM DATA WHERE SPECTRUM_ID = 0 AND DATA_TYPE = 1;"
          "DELETE FROM DATA WHERE CHROMATOGRAM_ID = 0 AND DATA_TYPE = 1;");
      MzMLSqliteHandler handler(filename, 0);
      std::vector<MSSpectrum> read_spectra;
      TEST_EXCEPTION(Exception::IllegalArgument, handler.readSpectra(read_spectra, {0}))
      std::vector<MSChromatogram> read_chroms;
      TEST_EXCEPTION(Exception::IllegalArgument, handler.readChromatograms(read_chroms, {0}))
    }

    // record 0 with two m/z (RT) arrays and no intensity array
    {
      STATUS((lossy ? "lossy" : "lossless") << ": record 0 with two m/z (RT) arrays and no intensity array")
      std::string filename;
      NEW_TMP_FILE_EXT(filename, extension);
      writeAltered(filename,
          "UPDATE DATA SET DATA_TYPE = 0 WHERE SPECTRUM_ID = 0 AND DATA_TYPE = 1;"
          "UPDATE DATA SET DATA_TYPE = 2 WHERE CHROMATOGRAM_ID = 0 AND DATA_TYPE = 1;");
      MzMLSqliteHandler handler(filename, 0);
      std::vector<MSSpectrum> read_spectra;
      TEST_EXCEPTION(Exception::IllegalArgument, handler.readSpectra(read_spectra, {0}))
      std::vector<MSChromatogram> read_chroms;
      TEST_EXCEPTION(Exception::IllegalArgument, handler.readChromatograms(read_chroms, {0}))
    }

    // record 1 with two intensity arrays and no m/z (RT) array
    {
      STATUS((lossy ? "lossy" : "lossless") << ": record 1 with two intensity arrays and no m/z (RT) array")
      std::string filename;
      NEW_TMP_FILE_EXT(filename, extension);
      writeAltered(filename,
          "UPDATE DATA SET DATA_TYPE = 1 WHERE SPECTRUM_ID = 1 AND DATA_TYPE = 0;"
          "UPDATE DATA SET DATA_TYPE = 1 WHERE CHROMATOGRAM_ID = 1 AND DATA_TYPE = 2;");
      MzMLSqliteHandler handler(filename, 0);
      std::vector<MSSpectrum> read_spectra;
      TEST_EXCEPTION(Exception::IllegalArgument, handler.readSpectra(read_spectra, {1}))
      std::vector<MSChromatogram> read_chroms;
      TEST_EXCEPTION(Exception::IllegalArgument, handler.readChromatograms(read_chroms, {1}))
    }
  }
}
END_SECTION

START_SECTION([EXTRA] reading ignores stored activation methods outside the enum)
{
  // The metadata readers skipped only -1 ("no activation method"), so -2 and below became
  // ActivationMethod values outside the range Precursor's name tables are indexed with, and
  // the 32 bit read wrapped larger stored values into that range.
  Precursor precursor;
  precursor.setMZ(500.25);
  precursor.getActivationMethods().insert(Precursor::ActivationMethod::CID);

  MSSpectrum spectrum;
  spectrum.setNativeID("spectrum=0");
  spectrum.setMSLevel(2);
  spectrum.push_back(Peak1D(100.0, 1000.0));
  spectrum.getPrecursors().push_back(precursor);

  MSChromatogram chrom;
  chrom.setNativeID("chromatogram=0");
  chrom.push_back(ChromatogramPeak(1.0, 10.0));
  chrom.setPrecursor(precursor);
  Product product;
  product.setMZ(250.5);
  chrom.setProduct(product);

  std::string filename;
  NEW_TMP_FILE(filename);
  std::filesystem::remove(filename);
  {
    MzMLSqliteHandler handler(filename, 0);
    handler.createTables();
    handler.writeSpectra({spectrum});
    handler.writeChromatograms({chrom});
  }

  // stores 'code' as the activation method of both precursors, returns the methods read back for
  // the spectrum and the chromatogram
  MzMLSqliteHandler handler(filename, 0);
  auto readBack = [&](const std::string& code)
  {
    {
      SqliteConnector conn(filename);
      conn.executeStatement("UPDATE PRECURSOR SET ACTIVATION_METHOD = " + code + ";");
    }
    std::vector<MSSpectrum> spectra;
    handler.readSpectra(spectra, {0}, true);
    std::vector<MSChromatogram> chroms;
    handler.readChromatograms(chroms, {0}, true);
    std::set<Precursor::ActivationMethod> spectrum_methods, chrom_methods;
    if (spectra.size() == 1 && spectra[0].getPrecursors().size() == 1)
    {
      spectrum_methods = spectra[0].getPrecursors()[0].getActivationMethods();
    }
    if (chroms.size() == 1)
    {
      chrom_methods = chroms[0].getPrecursor().getActivationMethods();
    }
    return std::make_pair(spectrum_methods, chrom_methods);
  };
  const std::set<Precursor::ActivationMethod> none;

  // as written
  auto methods = readBack(std::to_string(static_cast<int>(Precursor::ActivationMethod::CID)));
  TEST_EQUAL(methods.first == std::set<Precursor::ActivationMethod>{Precursor::ActivationMethod::CID}, true)
  TEST_EQUAL(methods.second == std::set<Precursor::ActivationMethod>{Precursor::ActivationMethod::CID}, true)
  // the largest valid code
  const int last_code = static_cast<int>(Precursor::ActivationMethod::SIZE_OF_ACTIVATIONMETHOD) - 1;
  methods = readBack(std::to_string(last_code));
  TEST_EQUAL(methods.first == std::set<Precursor::ActivationMethod>{static_cast<Precursor::ActivationMethod>(last_code)}, true)
  TEST_EQUAL(methods.second == std::set<Precursor::ActivationMethod>{static_cast<Precursor::ActivationMethod>(last_code)}, true)
  // -1 is what the writers store for no activation method
  methods = readBack("-1");
  TEST_EQUAL(methods.first == none, true)
  TEST_EQUAL(methods.second == none, true)
  methods = readBack("NULL");
  TEST_EQUAL(methods.first == none, true)
  TEST_EQUAL(methods.second == none, true)
  // invalid codes load without an activation method
  methods = readBack("-2");
  TEST_EQUAL(methods.first == none, true)
  TEST_EQUAL(methods.second == none, true)
  methods = readBack("-2147483648");
  TEST_EQUAL(methods.first == none, true)
  TEST_EQUAL(methods.second == none, true)
  methods = readBack(std::to_string(static_cast<int>(Precursor::ActivationMethod::SIZE_OF_ACTIVATIONMETHOD)));
  TEST_EQUAL(methods.first == none, true)
  TEST_EQUAL(methods.second == none, true)
  // 2^32 - 2 and 2^32 + 1 wrapped to -2 and 1 in a 32 bit read
  methods = readBack("4294967294");
  TEST_EQUAL(methods.first == none, true)
  TEST_EQUAL(methods.second == none, true)
  methods = readBack("4294967297");
  TEST_EQUAL(methods.first == none, true)
  TEST_EQUAL(methods.second == none, true)
}
END_SECTION

// reset error tolerances to default values
TOLERANCE_ABSOLUTE(1e-5)
TOLERANCE_RELATIVE(1+1e-5)

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST

