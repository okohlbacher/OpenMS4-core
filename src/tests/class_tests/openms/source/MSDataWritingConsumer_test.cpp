// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: $
// $Authors: $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>

///////////////////////////
#include <OpenMS/FORMAT/DATAACCESS/MSDataWritingConsumer.h>
///////////////////////////

#include <OpenMS/FORMAT/MzMLFile.h>
#include <OpenMS/KERNEL/MSExperiment.h>
#include <OpenMS/METADATA/DataProcessing.h>
#include <OpenMS/METADATA/SourceFile.h>

#include <fstream>
#include <iterator>
#include <string>

using namespace OpenMS;
using namespace std;

namespace
{
  // the consumer writes the footer and closes the file when it is destroyed, so the
  // file is only read back after the consumer has gone out of scope
  std::string readWholeFile(const std::string& filename)
  {
    std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
  }

  MSSpectrum makeSpectrum(const std::string& native_id, double rt)
  {
    MSSpectrum s;
    s.setNativeID(native_id);
    s.setRT(rt);
    s.setMSLevel(1);
    Peak1D p;
    p.setMZ(100.0);
    p.setIntensity(200.0f);
    s.push_back(p);
    return s;
  }

  MSChromatogram makeChromatogram(const std::string& native_id)
  {
    MSChromatogram c;
    c.setNativeID(native_id);
    ChromatogramPeak p;
    p.setRT(10.0);
    p.setIntensity(300.0f);
    c.push_back(p);
    return c;
  }

  template <typename Record>
  void addArrayHistories(Record& record, const std::string& prefix)
  {
    auto history = [&](auto& array, const std::string& type)
    {
      array.setName(prefix + type);
      auto processing = std::make_shared<DataProcessing>();
      processing->setMetaValue("array history", prefix + type);
      processing->getProcessingActions().insert(DataProcessing::SMOOTHING);
      array.getDataProcessing().push_back(processing);
    };
    record.getFloatDataArrays().resize(1);
    record.getIntegerDataArrays().resize(1);
    record.getStringDataArrays().resize(1);
    auto& floats = record.getFloatDataArrays()[0];
    auto& integers = record.getIntegerDataArrays()[0];
    auto& strings = record.getStringDataArrays()[0];
    floats.push_back(1.5f);
    integers.push_back(42);
    strings.push_back("label");
    history(floats, "float");
    history(integers, "integer");
    history(strings, "string");
  }
}

START_TEST(MSDataWritingConsumer, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

// MSDataWritingConsumer is abstract; PlainMSDataWritingConsumer is its implementation
// that writes spectra and chromatograms unchanged.
PlainMSDataWritingConsumer* ptr = nullptr;
PlainMSDataWritingConsumer* null_ptr = nullptr;
START_SECTION((MSDataWritingConsumer(const std::string& filename)))
{
  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);
  ptr = new PlainMSDataWritingConsumer(tmp_filename);
  TEST_NOT_EQUAL(ptr, null_ptr)
  TEST_EQUAL(ptr->getNrSpectraWritten(), 0)
  TEST_EQUAL(ptr->getNrChromatogramsWritten(), 0)
}
END_SECTION

START_SECTION((~MSDataWritingConsumer()))
{
  delete ptr;
}
END_SECTION

START_SECTION((void setExperimentalSettings(const ExperimentalSettings& exp)))
{
  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);
  {
    PlainMSDataWritingConsumer consumer(tmp_filename);
    ExperimentalSettings settings;
    settings.getSample().setName("streamed_sample");
    consumer.setExperimentalSettings(settings);
    consumer.setExpectedSize(1, 0);
    MSSpectrum s = makeSpectrum("spectrum=1", 10.0);
    consumer.consumeSpectrum(s);
  }
  PeakMap exp;
  MzMLFile().load(tmp_filename, exp);
  TEST_STRING_EQUAL(exp.getSample().getName(), "streamed_sample")
}
END_SECTION

START_SECTION((void setExpectedSize(Size expectedSpectra, Size expectedChromatograms)))
{
  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);
  {
    PlainMSDataWritingConsumer consumer(tmp_filename);
    consumer.setExpectedSize(2, 1);
    MSSpectrum s1 = makeSpectrum("spectrum=1", 10.0);
    MSSpectrum s2 = makeSpectrum("spectrum=2", 20.0);
    MSChromatogram c = makeChromatogram("TIC");
    consumer.consumeSpectrum(s1);
    consumer.consumeSpectrum(s2);
    consumer.consumeChromatogram(c);
  }
  const std::string content = readWholeFile(tmp_filename);
  TEST_NOT_EQUAL(content.find("<spectrumList count=\"2\""), std::string::npos)
  TEST_NOT_EQUAL(content.find("<chromatogramList count=\"1\""), std::string::npos)
}
END_SECTION

START_SECTION((void consumeSpectrum(SpectrumType & s)))
{
  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);
  {
    PlainMSDataWritingConsumer consumer(tmp_filename);
    consumer.setExpectedSize(2, 1);
    MSSpectrum s1 = makeSpectrum("spectrum=1", 10.0);
    MSSpectrum s2 = makeSpectrum("spectrum=2", 20.0);
    consumer.consumeSpectrum(s1);
    consumer.consumeSpectrum(s2);
    TEST_EQUAL(consumer.getNrSpectraWritten(), 2)

    MSChromatogram c = makeChromatogram("TIC");
    consumer.consumeChromatogram(c);
    // a second spectrumList cannot follow the chromatogramList
    TEST_EXCEPTION(Exception::IllegalArgument, consumer.consumeSpectrum(s1))
    TEST_EQUAL(consumer.getNrSpectraWritten(), 2)
  }
  PeakMap exp;
  MzMLFile().load(tmp_filename, exp);
  TEST_EQUAL(exp.size(), 2)
  ABORT_IF(exp.size() != 2)
  TEST_STRING_EQUAL(exp[1].getNativeID(), "spectrum=2")
  TEST_REAL_SIMILAR(exp[1].getRT(), 20.0)
  TEST_EQUAL(exp[1].size(), 1)
}
END_SECTION

START_SECTION((void consumeChromatogram(ChromatogramType & c)))
{
  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);
  {
    PlainMSDataWritingConsumer consumer(tmp_filename);
    consumer.setExpectedSize(0, 2);
    MSChromatogram c1 = makeChromatogram("TIC");
    MSChromatogram c2 = makeChromatogram("BPC");
    consumer.consumeChromatogram(c1);
    consumer.consumeChromatogram(c2);
    TEST_EQUAL(consumer.getNrChromatogramsWritten(), 2)
  }
  PeakMap exp;
  MzMLFile().load(tmp_filename, exp);
  TEST_EQUAL(exp.size(), 0)
  TEST_EQUAL(exp.getChromatograms().size(), 2)
  ABORT_IF(exp.getChromatograms().size() != 2)
  TEST_STRING_EQUAL(exp.getChromatograms()[1].getNativeID(), "BPC")
  TEST_EQUAL(exp.getChromatograms()[1].size(), 1)
}
END_SECTION

START_SECTION((virtual void addDataProcessing(DataProcessing d)))
{
  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);
  {
    PlainMSDataWritingConsumer consumer(tmp_filename);
    consumer.setExpectedSize(1, 0);
    DataProcessing dp;
    dp.getProcessingActions().insert(DataProcessing::SMOOTHING);
    consumer.addDataProcessing(dp);
    MSSpectrum s = makeSpectrum("spectrum=1", 10.0);
    consumer.consumeSpectrum(s);
    // the processing is added to the written copy, not to the caller's spectrum
    TEST_EQUAL(s.getDataProcessing().size(), 0)
  }
  PeakMap exp;
  MzMLFile().load(tmp_filename, exp);
  TEST_EQUAL(exp.size(), 1)
  ABORT_IF(exp.size() != 1)
  TEST_EQUAL(exp[0].getDataProcessing().size(), 1)
  ABORT_IF(exp[0].getDataProcessing().size() != 1)
  TEST_EQUAL(exp[0].getDataProcessing()[0]->getProcessingActions().count(DataProcessing::SMOOTHING), 1)
}
END_SECTION

START_SECTION((virtual Size getNrSpectraWritten()))
{
  NOT_TESTABLE // tested with consumeSpectrum
}
END_SECTION

START_SECTION((virtual Size getNrChromatogramsWritten()))
{
  NOT_TESTABLE // tested with consumeChromatogram
}
END_SECTION

START_SECTION(([EXTRA] later spectra only reference what the header written from the first spectrum declares))
{
  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);

  DataProcessing smoothing;
  smoothing.getProcessingActions().insert(DataProcessing::SMOOTHING);
  DataProcessing picking;
  picking.getProcessingActions().insert(DataProcessing::PEAK_PICKING);
  SourceFile second_source;
  second_source.setNameOfFile("second.raw");

  {
    PlainMSDataWritingConsumer consumer(tmp_filename);
    consumer.setExpectedSize(3, 0);

    MSSpectrum s1 = makeSpectrum("spectrum=1", 10.0);
    s1.getDataProcessing().push_back(DataProcessingPtr(new DataProcessing(smoothing)));
    consumer.consumeSpectrum(s1);

    // the same history as the first spectrum, held in objects of its own, and a source file
    MSSpectrum s2 = makeSpectrum("spectrum=2", 20.0);
    s2.getDataProcessing().push_back(DataProcessingPtr(new DataProcessing(smoothing)));
    s2.setSourceFile(second_source);
    consumer.consumeSpectrum(s2);

    // a history the header cannot declare any more
    MSSpectrum s3 = makeSpectrum("spectrum=3", 30.0);
    s3.getDataProcessing().push_back(DataProcessingPtr(new DataProcessing(picking)));
    consumer.consumeSpectrum(s3);
    TEST_EQUAL(consumer.getNrSpectraWritten(), 3)
  }

  // the header declares dp_sp_0 and no source file of any spectrum
  const std::string content = readWholeFile(tmp_filename);
  TEST_EQUAL(content.find("sourceFileRef=\"sf_sp_1\""), std::string::npos)
  TEST_EQUAL(content.find("dataProcessingRef=\"dp_sp_1\""), std::string::npos)
  TEST_EQUAL(content.find("dataProcessingRef=\"dp_sp_2\""), std::string::npos)

  PeakMap exp;
  MzMLFile().load(tmp_filename, exp);
  TEST_EQUAL(exp.size(), 3)
  ABORT_IF(exp.size() != 3)
  // the equal history is kept: the dangling reference used to read back as no history at all
  TEST_EQUAL(exp[1].getDataProcessing().size(), 1)
  ABORT_IF(exp[1].getDataProcessing().size() != 1)
  TEST_EQUAL(exp[1].getDataProcessing()[0]->getProcessingActions().count(DataProcessing::SMOOTHING), 1)
  TEST_EQUAL(exp[1].getSourceFile() == SourceFile(), true)
  // the undeclarable history falls back to the declared default
  TEST_EQUAL(exp[2].getDataProcessing().size(), 1)
  ABORT_IF(exp[2].getDataProcessing().size() != 1)
  TEST_EQUAL(exp[2].getDataProcessing()[0]->getProcessingActions().count(DataProcessing::SMOOTHING), 1)
}
END_SECTION

START_SECTION((regression: stored spectrum and chromatogram array histories are distinct))
{
  auto spectrum = makeSpectrum("spectrum=1", 10.0);
  auto chromatogram = makeChromatogram("TIC");
  addArrayHistories(spectrum, "spectrum ");
  addArrayHistories(chromatogram, "chromatogram ");
  PeakMap input;
  input.addSpectrum(spectrum);
  input.addChromatogram(chromatogram);
  std::string filename;
  NEW_TMP_FILE(filename)
  MzMLFile file;
  file.store(filename, input);
  TEST_EQUAL(file.isValid(filename), true)
  PeakMap loaded;
  file.load(filename, loaded);
  ABORT_IF(loaded.size() != 1 || loaded.getChromatograms().size() != 1)
  auto check = [&](const auto& record, const std::string& prefix)
  {
    const bool arrays_present = record.getFloatDataArrays().size() == 1 && record.getIntegerDataArrays().size() == 1 && record.getStringDataArrays().size() == 1;
    TEST_EQUAL(arrays_present, true)
    if (!arrays_present) { return; }
    const auto& floats = record.getFloatDataArrays()[0];
    const auto& integers = record.getIntegerDataArrays()[0];
    const auto& strings = record.getStringDataArrays()[0];
    const bool histories_present = floats.getDataProcessing().size() == 1 && integers.getDataProcessing().size() == 1 && strings.getDataProcessing().size() == 1;
    TEST_EQUAL(histories_present, true)
    if (!histories_present) { return; }
    TEST_STRING_EQUAL(floats.getDataProcessing()[0]->getMetaValue("array history").toString(), prefix + "float")
    TEST_STRING_EQUAL(integers.getDataProcessing()[0]->getMetaValue("array history").toString(), prefix + "integer")
    TEST_STRING_EQUAL(strings.getDataProcessing()[0]->getMetaValue("array history").toString(), prefix + "string")
    const bool values_present = floats.size() == 1 && integers.size() == 1 && strings.size() == 1;
    TEST_EQUAL(values_present, true)
    if (!values_present) { return; }
    TEST_REAL_SIMILAR(floats[0], 1.5)
    TEST_EQUAL(integers[0], 42)
    TEST_STRING_EQUAL(strings[0], "label")
  };
  check(loaded[0], "spectrum ");
  check(loaded.getChromatograms()[0], "chromatogram ");
}
END_SECTION

START_SECTION((regression: streamed arrays reference only declared processing records))
{
  for (bool spectrum_first : {false, true})
  {
    std::string filename;
    NEW_TMP_FILE(filename)
    {
      PlainMSDataWritingConsumer consumer(filename);
      consumer.setExpectedSize(spectrum_first ? 2 : 0, 2);
      if (spectrum_first)
      {
        auto first = makeSpectrum("spectrum=1", 10.0);
        addArrayHistories(first, "first ");
        consumer.consumeSpectrum(first);
        auto second = makeSpectrum("spectrum=2", 20.0);
        addArrayHistories(second, "second ");
        consumer.consumeSpectrum(second);
        TEST_EQUAL(second.getIntegerDataArrays()[0].getDataProcessing().size(), 1)
      }
      auto first = makeChromatogram("TIC");
      addArrayHistories(first, "first ");
      consumer.consumeChromatogram(first);
      auto second = makeChromatogram("BPC");
      addArrayHistories(second, "second ");
      consumer.consumeChromatogram(second);
      TEST_EQUAL(second.getFloatDataArrays()[0].getDataProcessing().size(), 1)
    }
    MzMLFile file;
    TEST_EQUAL(file.isValid(filename), true)
    const auto xml = readWholeFile(filename);
    TEST_EQUAL(xml.find("dataProcessingRef=\"dp_sp_1_"), std::string::npos)
    TEST_EQUAL(xml.find("dataProcessingRef=\"dp_ch_1_"), std::string::npos)
    TEST_EQUAL(xml.find("dataProcessingRef=\"dp_ch_0_") == std::string::npos, spectrum_first)
    PeakMap loaded;
    file.load(filename, loaded);
    TEST_EQUAL(loaded.size(), spectrum_first ? 2 : 0)
    TEST_EQUAL(loaded.getChromatograms().size(), 2)
  }
}
END_SECTION

END_TEST
