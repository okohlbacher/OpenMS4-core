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
#include <OpenMS/ANALYSIS/OPENSWATH/DATAACCESS/SpectrumAccessSqMass.h>
///////////////////////////

#include <OpenMS/ANALYSIS/OPENSWATH/DATAACCESS/SimpleOpenMSSpectraAccessFactory.h>
#include <OpenMS/FORMAT/DATAACCESS/MSDataSqlConsumer.h>
#include <OpenMS/FORMAT/HANDLERS/MzMLSqliteHandler.h>
#include <OpenMS/FORMAT/HANDLERS/MzMLSqliteSwathHandler.h>
#include <OpenMS/FORMAT/SqliteConnector.h>

using namespace OpenMS;
using namespace std;

START_TEST(SpectrumAccessSqMass, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

SpectrumAccessSqMass* ptr = nullptr;
SpectrumAccessSqMass* nullPointer = nullptr;

std::shared_ptr<PeakMap > exp(new PeakMap);
OpenSwath::SpectrumAccessPtr expptr = SimpleOpenMSSpectraFactory::getSpectrumAccessOpenMSPtr(exp);


START_SECTION(SpectrumAccessSqMass(OpenMS::Internal::MzMLSqliteHandler handler))
{
  OpenMS::Internal::MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  ptr = new SpectrumAccessSqMass(handler);
  TEST_NOT_EQUAL(ptr, nullPointer)
  delete ptr;
}
END_SECTION

    

START_SECTION(SpectrumAccessSqMass(const OpenMS::Internal::MzMLSqliteHandler& handler, const std::vector<int> & indices))
{
  OpenMS::Internal::MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  std::vector<int> indices;
  indices.push_back(1);
  ptr = new SpectrumAccessSqMass(handler, indices);
  TEST_NOT_EQUAL(ptr, nullPointer)

  TEST_EQUAL(ptr->getNrSpectra(), 1)
  delete ptr;
}
END_SECTION

START_SECTION(SpectrumAccessSqMass(const SpectrumAccessSqMass& sp, const std::vector<int>& indices))
{
  OpenMS::Internal::MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  ptr = new SpectrumAccessSqMass(handler);
  TEST_NOT_EQUAL(ptr, nullPointer)
  TEST_EQUAL(ptr->getNrSpectra(), 2)
  delete ptr;

  SpectrumAccessSqMass sasm(handler);

  // select subset of the data (all two spectra)
  {
    std::vector<int> indices;
    indices.push_back(0);
    indices.push_back(1);

    ptr = new SpectrumAccessSqMass(sasm, indices);
    TEST_NOT_EQUAL(ptr, nullPointer)
    TEST_EQUAL(ptr->getNrSpectra(), 2)
    delete ptr;
  }

  // select subset of the data (only second spectrum)
  {
    std::vector<int> indices;
    indices.push_back(1);

    ptr = new SpectrumAccessSqMass(sasm, indices);
    TEST_NOT_EQUAL(ptr, nullPointer)
    TEST_EQUAL(ptr->getNrSpectra(), 1)
  }

  // this should not work:
  // selecting subset of data (only second spectrum) shouldn't work if there is only a single spectrum
  {
    std::vector<int> indices;
    indices.push_back(1);
    TEST_EXCEPTION(Exception::IllegalArgument, new SpectrumAccessSqMass(*ptr, indices))

    std::vector<int> indices2;
    indices2.push_back(50);
    TEST_EXCEPTION(Exception::IllegalArgument, new SpectrumAccessSqMass(*ptr, indices2))

    std::vector<int> indices3;
    indices3.push_back(-1);
    TEST_EXCEPTION(Exception::IllegalArgument, new SpectrumAccessSqMass(*ptr, indices3))
  }
}
END_SECTION

START_SECTION(~SpectrumAccessSqMass())
{
  delete ptr;
}
END_SECTION

START_SECTION(size_t getNrSpectra() const)
{
  OpenMS::Internal::MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  ptr = new SpectrumAccessSqMass(handler);
  TEST_EQUAL(ptr->getNrSpectra(), 2)
  delete ptr;
}
END_SECTION

START_SECTION(OpenSwath::SpectrumPtr getSpectrumById(int id))
{
  OpenMS::Internal::MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  SpectrumAccessSqMass full(handler);
  TEST_EQUAL(full.getSpectrumById(0)->getMZArray()->data.size(), 19914)
  TEST_EQUAL(full.getSpectrumById(1)->getMZArray()->data.size(), 19800)

  std::vector<int> indices;
  indices.push_back(1);
  SpectrumAccessSqMass subset(handler, indices);
  TEST_EQUAL(subset.getSpectrumById(0)->getMZArray()->data.size(), 19800)

  // positions outside the one-element view must not be looked up in the index vector
  TEST_EXCEPTION(Exception::IllegalArgument, subset.getSpectrumById(1))
  TEST_EXCEPTION(Exception::IllegalArgument, subset.getSpectrumById(-1))
}
END_SECTION

START_SECTION(OpenSwath::SpectrumMeta getSpectrumMetaById(int id) const)
{
  OpenMS::Internal::MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  SpectrumAccessSqMass full(handler);
  TEST_EQUAL(full.getSpectrumMetaById(0).index, 0)
  TEST_EQUAL(full.getSpectrumMetaById(1).index, 1)
  TEST_EQUAL(full.getSpectrumMetaById(1).id, "controllerType=0 controllerNumber=1 scan=2")
  TEST_REAL_SIMILAR(full.getSpectrumMetaById(1).RT, 0.4738)

  std::vector<int> indices;
  indices.push_back(1);
  SpectrumAccessSqMass subset(handler, indices);
  // the index is the position in the view, the record still the second spectrum
  TEST_EQUAL(subset.getSpectrumMetaById(0).index, 0)
  TEST_EQUAL(subset.getSpectrumMetaById(0).id, "controllerType=0 controllerNumber=1 scan=2")

  TEST_EXCEPTION(Exception::IllegalArgument, subset.getSpectrumMetaById(1))
  TEST_EXCEPTION(Exception::IllegalArgument, subset.getSpectrumMetaById(-1))
}
END_SECTION

START_SECTION(std::vector<std::size_t> getSpectraByRT(double RT, double deltaRT) const)
{
  OpenMS::Internal::MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  // the two spectra are at RT 0.2961 and 0.4738
  SpectrumAccessSqMass full(handler);
  std::vector<std::size_t> res = full.getSpectraByRT(0.3, 0.2);
  TEST_EQUAL(res.size(), 2)

  // a zero width returns the first spectrum at or after RT, not only spectra at exactly RT
  res = full.getSpectraByRT(0.3, 0.0);
  TEST_EQUAL(res.size(), 1)
  TEST_EQUAL(res[0], 1)
  res = full.getSpectraByRT(0.5, 0.0);
  TEST_EQUAL(res.size(), 0)

  std::vector<int> indices;
  indices.push_back(1);
  SpectrumAccessSqMass subset(handler, indices);
  res = subset.getSpectraByRT(0.3, 0.0);
  TEST_EQUAL(res.size(), 1)
  TEST_EQUAL(res[0], 0)
}
END_SECTION

START_SECTION(std::shared_ptr<OpenSwath::ISpectrumAccess> lightClone() const)
{
  OpenMS::Internal::MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  ptr = new SpectrumAccessSqMass(handler);
  TEST_EQUAL(ptr->getNrSpectra(), 2)

  std::shared_ptr<OpenSwath::ISpectrumAccess> ptr2 = ptr->lightClone();
  TEST_EQUAL(ptr2->getNrSpectra(), 2)
  delete ptr;
}
END_SECTION

START_SECTION(void getAllSpectra(std::vector< OpenSwath::SpectrumPtr > & spectra, std::vector< OpenSwath::SpectrumMeta > & spectra_meta))
{
  OpenMS::Internal::MzMLSqliteHandler handler(OPENMS_GET_TEST_DATA_PATH("SqliteMassFile_1.sqMass"), 0);

  {
    ptr = new SpectrumAccessSqMass(handler);
    TEST_EQUAL(ptr->getNrSpectra(), 2)

    std::vector< OpenSwath::SpectrumPtr > spectra;
    std::vector< OpenSwath::SpectrumMeta > spectra_meta;
    ptr->getAllSpectra(spectra, spectra_meta);

    TEST_EQUAL(spectra.size(), 2)
    TEST_EQUAL(spectra_meta.size(), 2)

    TEST_EQUAL(spectra[0]->getMZArray()->data.size(), 19914)
    TEST_EQUAL(spectra[0]->getIntensityArray()->data.size(), 19914)

    TEST_EQUAL(spectra[1]->getMZArray()->data.size(), 19800)
    TEST_EQUAL(spectra[1]->getIntensityArray()->data.size(), 19800)

    TEST_EQUAL(spectra_meta[0].index, 0)
    TEST_EQUAL(spectra_meta[1].index, 1)
    delete ptr;
  }

  {
    std::vector<int> indices;
    indices.push_back(0);
    indices.push_back(1);

    ptr = new SpectrumAccessSqMass(handler, indices);
    TEST_EQUAL(ptr->getNrSpectra(), 2)

    std::vector< OpenSwath::SpectrumPtr > spectra;
    std::vector< OpenSwath::SpectrumMeta > spectra_meta;
    ptr->getAllSpectra(spectra, spectra_meta);

    TEST_EQUAL(spectra.size(), 2)
    TEST_EQUAL(spectra_meta.size(), 2)

    TEST_EQUAL(spectra[0]->getMZArray()->data.size(), 19914)
    TEST_EQUAL(spectra[0]->getIntensityArray()->data.size(), 19914)

    TEST_EQUAL(spectra[1]->getMZArray()->data.size(), 19800)
    TEST_EQUAL(spectra[1]->getIntensityArray()->data.size(), 19800)
    delete ptr;
  }

  // select only 2nd spectrum 
  {
    std::vector<int> indices;
    indices.push_back(1);

    ptr = new SpectrumAccessSqMass(handler, indices);
    TEST_EQUAL(ptr->getNrSpectra(), 1)

    std::vector< OpenSwath::SpectrumPtr > spectra;
    std::vector< OpenSwath::SpectrumMeta > spectra_meta;
    ptr->getAllSpectra(spectra, spectra_meta);

    TEST_EQUAL(spectra.size(), 1)
    TEST_EQUAL(spectra_meta.size(), 1)

    TEST_EQUAL(spectra[0]->getMZArray()->data.size(), 19800)
    TEST_EQUAL(spectra[0]->getIntensityArray()->data.size(), 19800)
    delete ptr;
  }

  // a view in reverse order and with a repeated spectrum is returned as configured,
  // matching what the single-spectrum accessors return for each position
  {
    std::vector<int> indices;
    indices.push_back(1);
    indices.push_back(0);
    indices.push_back(1);

    ptr = new SpectrumAccessSqMass(handler, indices);
    TEST_EQUAL(ptr->getNrSpectra(), 3)

    std::vector< OpenSwath::SpectrumPtr > spectra;
    std::vector< OpenSwath::SpectrumMeta > spectra_meta;
    ptr->getAllSpectra(spectra, spectra_meta);

    TEST_EQUAL(spectra.size(), 3)
    TEST_EQUAL(spectra_meta.size(), 3)

    TEST_EQUAL(spectra[0]->getMZArray()->data.size(), 19800)
    TEST_EQUAL(spectra[1]->getMZArray()->data.size(), 19914)
    TEST_EQUAL(spectra[2]->getMZArray()->data.size(), 19800)

    for (Size k = 0; k < 3; ++k)
    {
      TEST_EQUAL(spectra_meta[k].index, k)
      TEST_EQUAL(spectra_meta[k].id, ptr->getSpectrumMetaById((int)k).id)
      TEST_EQUAL(spectra[k]->getIntensityArray()->data.size(), ptr->getSpectrumById((int)k)->getIntensityArray()->data.size())
    }
    delete ptr;
  }

  // select only 2nd spectrum iteratively
  {

    std::vector<int> indices;
    indices.push_back(1);

    SpectrumAccessSqMass* sasm = new SpectrumAccessSqMass(handler, indices);
    TEST_EQUAL(sasm->getNrSpectra(), 1)

    // now we have an interface with a single spectrum in it, so if we select
    // the first spectrum of THAT interface, it should be the 2nd spectrum from
    // the initial dataset
    indices.clear();
    // indices.push_back(1); // this should not work as we now have only a single spectrum (out of bounds access!)
    indices.push_back(0);

    ptr = new SpectrumAccessSqMass(*sasm, indices);
    TEST_EQUAL(ptr->getNrSpectra(), 1)

    std::vector< OpenSwath::SpectrumPtr > spectra;
    std::vector< OpenSwath::SpectrumMeta > spectra_meta;
    ptr->getAllSpectra(spectra, spectra_meta);

    TEST_EQUAL(spectra.size(), 1)
    TEST_EQUAL(spectra_meta.size(), 1)

    TEST_EQUAL(spectra[0]->getMZArray()->data.size(), 19800)
    TEST_EQUAL(spectra[0]->getIntensityArray()->data.size(), 19800)
    delete ptr;
    delete sasm;
  }
}
END_SECTION

START_SECTION([EXTRA] reading spectra written by MSDataSqlConsumer)
{
  // lossy (numpress) compression with a mass accuracy used to store one- and
  // two-point m/z arrays with a fixed point of zero, which decoded as NaN
  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);
  {
    MSSpectrum one_peak;
    one_peak.setNativeID("one_peak");
    one_peak.setRT(10.0);
    one_peak.push_back(Peak1D(100.0, 1000.0));

    MSSpectrum two_peaks;
    two_peaks.setNativeID("two_peaks");
    two_peaks.setRT(20.0);
    two_peaks.push_back(Peak1D(100.0, 500.0));
    two_peaks.push_back(Peak1D(101.0, 600.0));

    MSDataSqlConsumer consumer(tmp_filename, 0, 500, false, true, 1e-4);
    consumer.consumeSpectrum(one_peak);
    consumer.consumeSpectrum(two_peaks);
    consumer.finalize();
  }

  // a negative buffer size is rejected before the existing file is replaced
  TEST_EXCEPTION(Exception::IllegalArgument, new MSDataSqlConsumer(tmp_filename, 0, -1))

  OpenMS::Internal::MzMLSqliteHandler handler(tmp_filename, 0);
  SpectrumAccessSqMass sasm(handler);
  TEST_EQUAL(sasm.getNrSpectra(), 2)

  OpenSwath::SpectrumPtr s0 = sasm.getSpectrumById(0);
  TEST_EQUAL(s0->getMZArray()->data.size(), 1)
  TEST_EQUAL(s0->getIntensityArray()->data.size(), 1)
  TEST_REAL_SIMILAR(s0->getMZArray()->data[0], 100.0)

  OpenSwath::SpectrumPtr s1 = sasm.getSpectrumById(1);
  TEST_EQUAL(s1->getMZArray()->data.size(), 2)
  TEST_EQUAL(s1->getIntensityArray()->data.size(), 2)
  TEST_REAL_SIMILAR(s1->getMZArray()->data[0], 100.0)
  TEST_REAL_SIMILAR(s1->getMZArray()->data[1], 101.0)

  std::vector< OpenSwath::SpectrumPtr > spectra;
  std::vector< OpenSwath::SpectrumMeta > spectra_meta;
  sasm.getAllSpectra(spectra, spectra_meta);
  TEST_EQUAL(spectra.size(), 2)
  TEST_EQUAL(spectra_meta.size(), 2)
  if (spectra.size() == 2 && spectra_meta.size() == 2 && spectra[1]->getMZArray()->data.size() == 2)
  {
    TEST_EQUAL(spectra_meta[1].id, "two_peaks")
    TEST_REAL_SIMILAR(spectra[1]->getMZArray()->data[1], 101.0)
  }

  // with full meta-data the experimental settings handed to the consumer are stored
  std::string meta_filename;
  NEW_TMP_FILE(meta_filename);
  {
    MSSpectrum spectrum;
    // a PSI-style id: the full meta-data snapshot is stored as mzML, whose writer renames every
    // spectrum to spectrum=<index> when any id lacks '=', and the reader then rejects the file
    spectrum.setNativeID("scan=1");
    spectrum.setRT(5.0);
    spectrum.push_back(Peak1D(200.0, 10.0));
    spectrum.push_back(Peak1D(201.0, 20.0));
    spectrum.push_back(Peak1D(202.0, 30.0));

    ExperimentalSettings settings;
    settings.getSample().setName("MSDataSqlConsumer sample");

    MSDataSqlConsumer consumer(meta_filename, 0, 500, true);
    consumer.setExperimentalSettings(settings);
    consumer.consumeSpectrum(spectrum);
  } // the destructor finalizes the file

  MSExperiment meta_exp;
  OpenMS::Internal::MzMLSqliteHandler meta_handler(meta_filename, 0);
  meta_handler.readExperiment(meta_exp);
  TEST_EQUAL(meta_exp.getNrSpectra(), 1)
  TEST_EQUAL(meta_exp.getSample().getName(), "MSDataSqlConsumer sample")

  // a run registered with addRun() still stores the full meta-data of its records
  std::string run_filename;
  NEW_TMP_FILE(run_filename);
  {
    MSSpectrum spectrum;
    spectrum.setNativeID("scan=1"); // the snapshot is stored as mzML, whose writer renames ids without '='
    spectrum.setRT(7.0);
    spectrum.push_back(Peak1D(300.0, 10.0));
    spectrum.push_back(Peak1D(301.0, 20.0));
    // MetaInfo is carried only by the snapshot, never by the SQL fallback
    spectrum.setMetaValue("addRun_marker", "kept");

    ExperimentalSettings settings;
    settings.getSample().setName("addRun sample");

    MSDataSqlConsumer consumer(run_filename, 0, 500, true);
    consumer.setExperimentalSettings(settings);
    consumer.addRun("input_run.mzML", 5);
    consumer.consumeSpectrum(spectrum);
  } // the destructor finalizes the file

  MSExperiment run_exp;
  OpenMS::Internal::MzMLSqliteHandler run_handler(run_filename, 5);
  run_handler.readExperiment(run_exp);
  TEST_EQUAL(run_exp.getNrSpectra(), 1)
  TEST_EQUAL(run_exp.getSample().getName(), "addRun sample")
  ABORT_IF(run_exp.getNrSpectra() != 1)
  TEST_EQUAL(run_exp[0].getMetaValue("addRun_marker").toString(), "kept")
}
END_SECTION

START_SECTION([EXTRA] MSDataSqlConsumer keeps the full meta-data of records consumed before addRun())
{
  // The reader pairs the SQL records with the headers of the full meta-data snapshot by
  // position, so the snapshot of the run registered by addRun() has to cover the records
  // consumed before it as well; with only the later records, the file could not be read.
  std::string filename;
  NEW_TMP_FILE(filename);
  {
    MSSpectrum before;
    before.setNativeID("scan=1"); // the snapshot is stored as mzML, whose writer renames ids without '='
    before.setRT(3.0);
    before.push_back(Peak1D(400.0, 10.0));
    before.setMetaValue("consumed", "before addRun");

    MSSpectrum after;
    after.setNativeID("scan=2");
    after.setRT(4.0);
    after.push_back(Peak1D(410.0, 20.0));
    after.push_back(Peak1D(411.0, 30.0));
    after.setMetaValue("consumed", "after addRun");

    MSDataSqlConsumer consumer(filename, 0, 500, true);
    consumer.consumeSpectrum(before);
    consumer.addRun("input_run.mzML", 5);
    consumer.consumeSpectrum(after);
  } // the destructor finalizes the file

  MSExperiment exp;
  OpenMS::Internal::MzMLSqliteHandler handler(filename, 5);
  handler.readExperiment(exp);
  TEST_EQUAL(exp.getNrSpectra(), 2)
  ABORT_IF(exp.getNrSpectra() != 2)
  TEST_EQUAL(exp[0].getNativeID(), "scan=1")
  TEST_EQUAL(exp[0].getMetaValue("consumed").toString(), "before addRun")
  TEST_EQUAL(exp[0].size(), 1)
  TEST_EQUAL(exp[1].getNativeID(), "scan=2")
  TEST_EQUAL(exp[1].getMetaValue("consumed").toString(), "after addRun")
  TEST_EQUAL(exp[1].size(), 2)
}
END_SECTION

START_SECTION([EXTRA] MSDataSqlConsumer describes a run registered after a written run by its own records only)
{
  // With full_meta, the snapshot of a run registered by addRun() (its RUN_EXTRA entry) holds the
  // headers of the records consumed for it. Once a run has been written, the next addRun() starts
  // the headers over, also when no snapshot is pending then: after an explicit finalize(), or after
  // an addRun() that wrote the pending snapshot and then failed on its RUN entry. Each case consumes
  // 'scan=1' for an earlier run and 'scan=2' for run 6. readExperiment rejects a file with more
  // than one run and pairs the data of all runs with one snapshot, so the RUN rows of the other runs
  // are removed and only the meta-data is read, which is the snapshot of the kept run as stored.
  auto describedBy = [](const std::string& filename, UInt64 run_id)
  {
    {
      SqliteConnector conn(filename);
      conn.executeStatement("DELETE FROM RUN WHERE ID <> " + std::to_string(run_id) + ";");
    }
    MSExperiment exp;
    OpenMS::Internal::MzMLSqliteHandler handler(filename, run_id);
    handler.readExperiment(exp, true);
    // native id and the MetaValue that only the snapshot carries (the SQL fallback has none)
    std::string described;
    for (const MSSpectrum& s : exp.getSpectra())
    {
      described += (described.empty() ? "" : " ") + s.getNativeID() + ":" + s.getMetaValue("consumed", "").toString();
    }
    return described;
  };
  auto makeSpectrum = [](const std::string& native_id, const std::string& consumed)
  {
    MSSpectrum s;
    s.setNativeID(native_id); // the snapshot is stored as mzML, whose writer renames ids without '='
    s.setRT(5.0);
    s.push_back(Peak1D(500.0, 10.0));
    s.setMetaValue("consumed", consumed);
    return s;
  };

  // addRun(5), 'scan=1', finalize() (writes the snapshot of run 5), addRun(6), 'scan=2'
  auto writeFinalizedRun = [&makeSpectrum](const std::string& filename)
  {
    MSSpectrum first = makeSpectrum("scan=1", "first");
    MSSpectrum second = makeSpectrum("scan=2", "second");
    MSDataSqlConsumer consumer(filename, 0, 500, true);
    consumer.addRun("first_run.mzML", 5);
    consumer.consumeSpectrum(first);
    consumer.finalize();
    consumer.addRun("second_run.mzML", 6);
    consumer.consumeSpectrum(second);
  }; // the destructor writes the snapshot of run 6
  std::string finalized_run6;
  NEW_TMP_FILE(finalized_run6);
  writeFinalizedRun(finalized_run6);
  TEST_STRING_EQUAL(describedBy(finalized_run6, 6), "scan=2:second")
  std::string finalized_run5;
  NEW_TMP_FILE(finalized_run5);
  writeFinalizedRun(finalized_run5);
  TEST_STRING_EQUAL(describedBy(finalized_run5, 5), "scan=1:first")

  // 'scan=1', finalize() (writes run 0 with its snapshot), addRun(6), 'scan=2'
  std::string finalized_first;
  NEW_TMP_FILE(finalized_first);
  {
    MSSpectrum first = makeSpectrum("scan=1", "first");
    MSSpectrum second = makeSpectrum("scan=2", "second");
    MSDataSqlConsumer consumer(finalized_first, 0, 500, true);
    consumer.consumeSpectrum(first);
    consumer.finalize();
    consumer.addRun("second_run.mzML", 6);
    consumer.consumeSpectrum(second);
  } // the destructor writes the snapshot of run 6
  TEST_STRING_EQUAL(describedBy(finalized_first, 6), "scan=2:second")

  // addRun(5), 'scan=1', addRun(5) (writes the snapshot of run 5, then fails on the RUN entry that
  // already exists), addRun(6), 'scan=2'
  std::string failed_run;
  NEW_TMP_FILE(failed_run);
  {
    MSSpectrum first = makeSpectrum("scan=1", "first");
    MSSpectrum second = makeSpectrum("scan=2", "second");
    MSDataSqlConsumer consumer(failed_run, 0, 500, true);
    consumer.addRun("first_run.mzML", 5);
    consumer.consumeSpectrum(first);
    TEST_EXCEPTION(Exception::IllegalArgument, consumer.addRun("first_run.mzML", 5))
    consumer.addRun("second_run.mzML", 6);
    consumer.consumeSpectrum(second);
  } // the destructor writes the snapshot of run 6
  TEST_STRING_EQUAL(describedBy(failed_run, 6), "scan=2:second")
}
END_SECTION

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST

