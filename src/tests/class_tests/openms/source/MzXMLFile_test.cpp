// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Marc Sturm, Chris Bielow $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/TestFileValidation.h>
#include <OpenMS/test_config.h>

///////////////////////////

#include <OpenMS/CONCEPT/LogStream.h>
#include <OpenMS/FORMAT/FileTypes.h>
#include <OpenMS/FORMAT/MzXMLFile.h>
#include <OpenMS/KERNEL/MSExperiment.h>
#include <OpenMS/KERNEL/StandardTypes.h>
#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <vector>

using namespace OpenMS;
using namespace std;

DRange<1> makeRange(double a, double b)
{
  DPosition<1> pa(a), pb(b);
  return DRange<1>(pa, pb);
}

class TICConsumer : 
    public Interfaces::IMSDataConsumer
{

    typedef PeakMap MapType;
    typedef MapType::SpectrumType SpectrumType;
    typedef MapType::ChromatogramType ChromatogramType;

public:
  double TIC;
  int nr_spectra;
  long int nr_peaks;

  // Create new consumer, set TIC to zero
  TICConsumer() :
    TIC(0.0),
    nr_spectra(0.0),
    nr_peaks(0)
    {}

  void consumeSpectrum(SpectrumType & s) override
  {
    for (Size i = 0; i < s.size(); i++) 
    { 
      TIC += s[i].getIntensity(); 
    }
    nr_peaks += s.size();
    nr_spectra++;
  }

  void consumeChromatogram(ChromatogramType& /* c */) override {}
  void setExpectedSize(Size /* expectedSpectra */, Size /* expectedChromatograms */) override {}
  void setExperimentalSettings(const ExperimentalSettings& /* exp */) override {}
};

///////////////////////////

START_TEST(MzXMLFile, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

MzXMLFile* ptr = nullptr;
MzXMLFile* nullPointer = nullptr;

START_SECTION((MzXMLFile()))
{
  ptr = new MzXMLFile;
  TEST_NOT_EQUAL(ptr, nullPointer)
}
END_SECTION

START_SECTION((~MzXMLFile()))
{
  delete ptr;
}
END_SECTION

START_SECTION(const PeakFileOptions& getOptions() const)
{
  MzXMLFile file;
  TEST_EQUAL(file.getOptions().hasMSLevels(),false)
}
END_SECTION

START_SECTION(PeakFileOptions& getOptions())
{
  MzXMLFile file;
  file.getOptions().addMSLevel(1);
  TEST_EQUAL(file.getOptions().hasMSLevels(),true);
}
END_SECTION

START_SECTION((template<typename MapType> void load(const std::string& filename, MapType& map) ))
{
  TOLERANCE_ABSOLUTE(0.01)

    MzXMLFile file;

  //exception
  PeakMap e;
  TEST_EXCEPTION( Exception::FileNotFound , file.load("dummy/dummy.mzXML",e) )

    //real test
    file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"),e);

  //test DocumentIdentifier addition
  TEST_STRING_EQUAL(e.getLoadedFilePath(), OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"));
  TEST_STRING_EQUAL(FileTypes::typeToName(e.getLoadedFileType()),"mzXML");

  //---------------------------------------------------------------------------
  // actual peak data
  // 60 : (120,100)
  // 120: (110,100) (120,200) (130,100)
  // 180: (100,100) (110,200) (120,300) (130,200) (140,100)
  //---------------------------------------------------------------------------
  TEST_EQUAL(e.size(), 4)
    TEST_EQUAL(e[0].getMSLevel(), 1)
    TEST_EQUAL(e[1].getMSLevel(), 1)
    TEST_EQUAL(e[2].getMSLevel(), 1)
    TEST_EQUAL(e[3].getMSLevel(), 2)
    TEST_EQUAL(e[0].size(), 1)
    TEST_EQUAL(e[1].size(), 3)
    TEST_EQUAL(e[2].size(), 5)
    TEST_EQUAL(e[3].size(), 5)
    TEST_STRING_EQUAL(e[0].getNativeID(),"scan=10")
    TEST_STRING_EQUAL(e[1].getNativeID(),"scan=11")
    TEST_STRING_EQUAL(e[2].getNativeID(),"scan=12")
    TEST_STRING_EQUAL(e[3].getNativeID(),"scan=13")

    TEST_REAL_SIMILAR(e[0][0].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e[0][0].getIntensity(), 100)
    TEST_REAL_SIMILAR(e[1][0].getPosition()[0], 110)
    TEST_REAL_SIMILAR(e[1][0].getIntensity(), 100)
    TEST_REAL_SIMILAR(e[1][1].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e[1][1].getIntensity(), 200)
    TEST_REAL_SIMILAR(e[1][2].getPosition()[0], 130)
    TEST_REAL_SIMILAR(e[1][2].getIntensity(), 100)
    TEST_REAL_SIMILAR(e[2][0].getPosition()[0], 100)
    TEST_REAL_SIMILAR(e[2][0].getIntensity(), 100)
    TEST_REAL_SIMILAR(e[2][1].getPosition()[0], 110)
    TEST_REAL_SIMILAR(e[2][1].getIntensity(), 200)
    TEST_REAL_SIMILAR(e[2][2].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e[2][2].getIntensity(), 300)
    TEST_REAL_SIMILAR(e[2][3].getPosition()[0], 130)
    TEST_REAL_SIMILAR(e[2][3].getIntensity(), 200)
    TEST_REAL_SIMILAR(e[2][4].getPosition()[0], 140)
    TEST_REAL_SIMILAR(e[2][4].getIntensity(), 100)

    TEST_EQUAL(e[0].getMetaValue("URL1"), "www.open-ms.de")
    TEST_EQUAL(e[0].getMetaValue("URL2"), "www.uni-tuebingen.de")
    TEST_EQUAL(e[0].getComment(), "Scan Comment")

    //---------------------------------------------------------------------------
    // source file
    //---------------------------------------------------------------------------
    TEST_EQUAL(e.getSourceFiles().size(),2)
    TEST_STRING_EQUAL(e.getSourceFiles()[0].getNameOfFile(), "File_test_1.raw");
  TEST_STRING_EQUAL(e.getSourceFiles()[0].getPathToFile(), "");
  TEST_STRING_EQUAL(e.getSourceFiles()[0].getFileType(), "RAWData");
  TEST_STRING_EQUAL(e.getSourceFiles()[0].getChecksum(), "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
  TEST_EQUAL(e.getSourceFiles()[0].getChecksumType(),SourceFile::ChecksumType::SHA1)
    TEST_STRING_EQUAL(e.getSourceFiles()[1].getNameOfFile(), "File_test_2.raw");
  TEST_STRING_EQUAL(e.getSourceFiles()[1].getPathToFile(), "");
  TEST_STRING_EQUAL(e.getSourceFiles()[1].getFileType(), "processedData");
  TEST_STRING_EQUAL(e.getSourceFiles()[1].getChecksum(), "2fd4e1c67a2d28fced849ee1bb76e7391b93eb13");
  TEST_EQUAL(e.getSourceFiles()[1].getChecksumType(),SourceFile::ChecksumType::SHA1)

    //---------------------------------------------------------------------------
    // data processing (assigned to each spectrum)
    //---------------------------------------------------------------------------
    for (Size i=0; i<e.size(); ++i)
    {
      TEST_EQUAL(e[i].getDataProcessing().size(),2)

        TEST_EQUAL(e[i].getDataProcessing()[0]->getSoftware().getName(), "MS-X");
      TEST_EQUAL(e[i].getDataProcessing()[0]->getSoftware().getVersion(), "1.0");
      TEST_STRING_EQUAL(e[i].getDataProcessing()[0]->getMetaValue("#type").toString(), "conversion");
      TEST_STRING_EQUAL(e[i].getDataProcessing()[0]->getMetaValue("processing 1").toString(), "done 1");
      TEST_STRING_EQUAL(e[i].getDataProcessing()[0]->getMetaValue("processing 2").toString(), "done 2");
      TEST_EQUAL(e[i].getDataProcessing()[0]->getCompletionTime().get(), "2001-02-03 04:05:06");
      TEST_EQUAL(e[i].getDataProcessing()[0]->getProcessingActions().size(),0)

        TEST_EQUAL(e[i].getDataProcessing()[1]->getSoftware().getName(), "MS-Y");
      TEST_EQUAL(e[i].getDataProcessing()[1]->getSoftware().getVersion(), "1.1");
      TEST_STRING_EQUAL(e[i].getDataProcessing()[1]->getMetaValue("#type").toString(), "processing");
      TEST_REAL_SIMILAR((double)(e[i].getDataProcessing()[1]->getMetaValue("#intensity_cutoff")), 3.4);
      TEST_STRING_EQUAL(e[i].getDataProcessing()[1]->getMetaValue("processing 3").toString(), "done 3");
      TEST_EQUAL(e[i].getDataProcessing()[1]->getCompletionTime().get(), "0000-00-00 00:00:00");
      TEST_EQUAL(e[i].getDataProcessing()[1]->getProcessingActions().size(),3)
        TEST_EQUAL(e[i].getDataProcessing()[1]->getProcessingActions().count(DataProcessing::DEISOTOPING),1)
        TEST_EQUAL(e[i].getDataProcessing()[1]->getProcessingActions().count(DataProcessing::CHARGE_DECONVOLUTION),1)
        TEST_EQUAL(e[i].getDataProcessing()[1]->getProcessingActions().count(DataProcessing::PEAK_PICKING),1)
    }

  //---------------------------------------------------------------------------
  // instrument
  //---------------------------------------------------------------------------
  const Instrument& inst = e.getInstrument();
  TEST_EQUAL(inst.getVendor(), "MS-Vendor")
    TEST_EQUAL(inst.getModel(), "MS 1")
    TEST_EQUAL(inst.getMetaValue("URL1"), "www.open-ms.de")
    TEST_EQUAL(inst.getMetaValue("URL2"), "www.uni-tuebingen.de")
    TEST_EQUAL(inst.getMetaValue("#comment"), "Instrument Comment")
    TEST_EQUAL(inst.getName(), "")
    TEST_EQUAL(inst.getCustomizations(), "")
    TEST_EQUAL(inst.getIonSources().size(),1)
    TEST_EQUAL(inst.getIonSources()[0].getIonizationMethod(), IonSource::IonizationMethod::ESI)
    TEST_EQUAL(inst.getIonSources()[0].getInletType(), IonSource::InletType::INLETNULL)
    TEST_EQUAL(inst.getIonSources()[0].getPolarity(), IonSource::Polarity::POLNULL)
    TEST_EQUAL(inst.getIonDetectors().size(),1)
    TEST_EQUAL(inst.getIonDetectors()[0].getType(), IonDetector::Type::FARADAYCUP)
    TEST_REAL_SIMILAR(inst.getIonDetectors()[0].getResolution(), 0.0f)
    TEST_REAL_SIMILAR(inst.getIonDetectors()[0].getADCSamplingFrequency(), 0.0f)
    TEST_EQUAL(inst.getIonDetectors()[0].getAcquisitionMode(), IonDetector::AcquisitionMode::ACQMODENULL)
    TEST_EQUAL(inst.getMassAnalyzers().size(), 1)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getType(), MassAnalyzer::AnalyzerType::PAULIONTRAP)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getResolutionMethod(), MassAnalyzer::ResolutionMethod::FWHM)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getResolutionType(), MassAnalyzer::ResolutionType::RESTYPENULL)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getScanDirection(), MassAnalyzer::ScanDirection::SCANDIRNULL)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getScanLaw(), MassAnalyzer::ScanLaw::SCANLAWNULL)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getReflectronState(), MassAnalyzer::ReflectronState::REFLSTATENULL)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getResolution(), 0.0f)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getAccuracy(), 0.0f)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getScanRate(), 0.0f)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getScanTime(), 0.0f)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getTOFTotalPathLength(), 0.0f)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getIsolationWidth(), 0.0f)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getFinalMSExponent(), 0)
    TEST_EQUAL(inst.getMassAnalyzers()[0].getMagneticFieldStrength(), 0.0f)
    TEST_EQUAL(inst.getSoftware().getName(),"MS-Z")
    TEST_EQUAL(inst.getSoftware().getVersion(),"3.0")

    //---------------------------------------------------------------------------
    // contact persons
    //---------------------------------------------------------------------------
    const vector<ContactPerson>& contacts = e.getContacts();
  TEST_EQUAL(contacts.size(),1)
    TEST_STRING_EQUAL(contacts[0].getFirstName(),"FirstName")
    TEST_STRING_EQUAL(contacts[0].getLastName(),"LastName")
    TEST_STRING_EQUAL(contacts[0].getMetaValue("#phone"),"0049")
    TEST_STRING_EQUAL(contacts[0].getEmail(),"a@b.de")
    TEST_STRING_EQUAL(contacts[0].getURL(),"http://bla.de")
    TEST_STRING_EQUAL(contacts[0].getContactInfo(),"")

    //---------------------------------------------------------------------------
    // sample
    //---------------------------------------------------------------------------
    TEST_EQUAL(e.getSample().getName(), "")
    TEST_EQUAL(e.getSample().getNumber(), "")
    TEST_EQUAL(e.getSample().getState(), Sample::SampleState::SAMPLENULL)
    TEST_EQUAL(e.getSample().getMass(), 0.0f)
    TEST_EQUAL(e.getSample().getVolume(), 0.0f)
    TEST_EQUAL(e.getSample().getConcentration(), 0.0f)

    //---------------------------------------------------------------------------
    // precursors
    //---------------------------------------------------------------------------
    TEST_EQUAL(e[0].getPrecursors().size(),0)
    TEST_EQUAL(e[1].getPrecursors().size(),0)
    TEST_EQUAL(e[2].getPrecursors().size(),0)
    TEST_EQUAL(e[3].getPrecursors().size(),3)

    TEST_REAL_SIMILAR(e[3].getPrecursors()[0].getMZ(),101.0)
    TEST_REAL_SIMILAR(e[3].getPrecursors()[0].getIntensity(),100.0)
    TEST_REAL_SIMILAR(e[3].getPrecursors()[0].getIsolationWindowLowerOffset(),5)
    TEST_REAL_SIMILAR(e[3].getPrecursors()[0].getIsolationWindowUpperOffset(),5)
    TEST_EQUAL(e[3].getPrecursors()[0].getCharge(),1)

    TEST_REAL_SIMILAR(e[3].getPrecursors()[1].getMZ(),201.0)
    TEST_REAL_SIMILAR(e[3].getPrecursors()[1].getIntensity(),200.0)
    TEST_REAL_SIMILAR(e[3].getPrecursors()[1].getIsolationWindowLowerOffset(),10)
    TEST_REAL_SIMILAR(e[3].getPrecursors()[1].getIsolationWindowUpperOffset(),10)
    TEST_EQUAL(e[3].getPrecursors()[1].getCharge(),2)

    TEST_REAL_SIMILAR(e[3].getPrecursors()[2].getMZ(),301.0)
    TEST_REAL_SIMILAR(e[3].getPrecursors()[2].getIntensity(),300.0)
    TEST_REAL_SIMILAR(e[3].getPrecursors()[2].getIsolationWindowLowerOffset(),15)
    TEST_REAL_SIMILAR(e[3].getPrecursors()[2].getIsolationWindowUpperOffset(),15)
    TEST_EQUAL(e[3].getPrecursors()[2].getCharge(),3)

    /////////////////////// TESTING SPECIAL CASES ///////////////////////

    //load a second time to make sure everything is re-initialized correctly
    PeakMap e2;
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"),e2);
  TEST_EQUAL(e==e2,true)

    //test reading 64 bit data
    PeakMap e3;
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_3_64bit.mzXML"),e3);

  TEST_EQUAL(e3.size(), 3)
    TEST_EQUAL(e3[0].getMSLevel(), 1)
    TEST_EQUAL(e3[1].getMSLevel(), 1)
    TEST_EQUAL(e3[2].getMSLevel(), 1)
    TEST_REAL_SIMILAR(e3[0].getRT(), 1)
    TEST_REAL_SIMILAR(e3[1].getRT(), 121)
    TEST_REAL_SIMILAR(e3[2].getRT(), 3661)
    TEST_EQUAL(e3[0].size(), 1)
    TEST_EQUAL(e3[1].size(), 3)
    TEST_EQUAL(e3[2].size(), 5)

    TEST_REAL_SIMILAR(e3[0][0].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e3[0][0].getIntensity(), 100)
    TEST_REAL_SIMILAR(e3[1][0].getPosition()[0], 110)
    TEST_REAL_SIMILAR(e3[1][0].getIntensity(), 100)
    TEST_REAL_SIMILAR(e3[1][1].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e3[1][1].getIntensity(), 200)
    TEST_REAL_SIMILAR(e3[1][2].getPosition()[0], 130)
    TEST_REAL_SIMILAR(e3[1][2].getIntensity(), 100)
    TEST_REAL_SIMILAR(e3[2][0].getPosition()[0], 100)
    TEST_REAL_SIMILAR(e3[2][0].getIntensity(), 100)
    TEST_REAL_SIMILAR(e3[2][1].getPosition()[0], 110)
    TEST_REAL_SIMILAR(e3[2][1].getIntensity(), 200)
    TEST_REAL_SIMILAR(e3[2][2].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e3[2][2].getIntensity(), 300)
    TEST_REAL_SIMILAR(e3[2][3].getPosition()[0], 130)
    TEST_REAL_SIMILAR(e3[2][3].getIntensity(), 200)
    TEST_REAL_SIMILAR(e3[2][4].getPosition()[0], 140)
    TEST_REAL_SIMILAR(e3[2][4].getIntensity(), 100)

    //loading a minimal file containing one spectrum - with whitespaces inside the base64 data
    PeakMap e4;
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_2_minimal.mzXML"),e4);
  TEST_EQUAL(e4.size(),1)
    TEST_EQUAL(e4[0].size(),1)

    //load one extremely long spectrum - tests CDATA splitting
    PeakMap e5;
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_4_long.mzXML"),e5);
  TEST_EQUAL(e5.size(), 1)
    TEST_EQUAL(e5[0].size(), 997530)

    //zlib functionality
    PeakMap zlib;
  PeakMap none;
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"),none);
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1_compressed.mzXML"),zlib);
  TEST_EQUAL(zlib==none,true)
}
END_SECTION

START_SECTION(([EXTRA] load with metadata only flag))
{
  TOLERANCE_ABSOLUTE(0.01)

    PeakMap e;
  MzXMLFile file;
  file.getOptions().setMetadataOnly(true);

  // real test
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"),e);

  TEST_EQUAL(e.size(),0)
    TEST_EQUAL(e.getSourceFiles().size(),2)
    TEST_STRING_EQUAL(e.getSourceFiles()[0].getNameOfFile(), "File_test_1.raw");
  TEST_STRING_EQUAL(e.getSourceFiles()[0].getPathToFile(), "");
  TEST_EQUAL(e.getContacts().size(),1)
    TEST_STRING_EQUAL(e.getContacts()[0].getFirstName(),"FirstName")
    TEST_STRING_EQUAL( e.getContacts()[0].getLastName(),"LastName")
    TEST_STRING_EQUAL(e.getSample().getName(), "")
    TEST_STRING_EQUAL(e.getSample().getNumber(), "")
}
END_SECTION

START_SECTION(([EXTRA] load with selected MS levels))
{
  TOLERANCE_ABSOLUTE(0.01)

    PeakMap e;
  MzXMLFile file;

  // load only MS level 1
  file.getOptions().addMSLevel(1);
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"),e);

  TEST_EQUAL(e.size(), 3)
    TEST_EQUAL(e[0].getMSLevel(), 1)
    TEST_EQUAL(e[1].getMSLevel(), 1)
    TEST_EQUAL(e[2].getMSLevel(), 1)
    TEST_EQUAL(e[0].size(), 1)
    TEST_EQUAL(e[1].size(), 3)
    TEST_EQUAL(e[2].size(), 5)
    TEST_STRING_EQUAL(e[0].getNativeID(),"scan=10")
    TEST_STRING_EQUAL(e[1].getNativeID(),"scan=11")
    TEST_STRING_EQUAL(e[2].getNativeID(),"scan=12")

    // load all levels
    file.getOptions().clearMSLevels();
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"),e);

  TEST_EQUAL(e.size(), 4)
}
END_SECTION

START_SECTION(([EXTRA] load with selected MZ range))
{
  TOLERANCE_ABSOLUTE(0.01)

    PeakMap e;
  MzXMLFile file;

  file.getOptions().setMZRange(makeRange(115,135));
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"),e);
  //---------------------------------------------------------------------------
  // 60 : +(120,100)
  // 120: -(110,100) +(120,200) +(130,100)
  // 180: -(100,100) -(110,200) +(120,300) +(130,200) -(140,100)
  //---------------------------------------------------------------------------

  TEST_EQUAL(e[0].size(), 1)
    TEST_EQUAL(e[1].size(), 2)
    TEST_EQUAL(e[2].size(), 2)

    TEST_REAL_SIMILAR(e[0][0].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e[0][0].getIntensity(), 100)
    TEST_REAL_SIMILAR(e[1][0].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e[1][0].getIntensity(), 200)
    TEST_REAL_SIMILAR(e[1][1].getPosition()[0], 130)
    TEST_REAL_SIMILAR(e[1][1].getIntensity(), 100)
    TEST_REAL_SIMILAR(e[2][0].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e[2][0].getIntensity(), 300)
    TEST_REAL_SIMILAR(e[2][1].getPosition()[0], 130)
    TEST_REAL_SIMILAR(e[2][1].getIntensity(), 200)
}
END_SECTION

START_SECTION(([EXTRA] load with RT range))
{
  TOLERANCE_ABSOLUTE(0.01)

    PeakMap e;
  MzXMLFile file;
  file.getOptions().setRTRange(makeRange(100, 200));
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"),e);
  //---------------------------------------------------------------------------
  // 120: (110,100) (120,200) (130,100)
  // 180: (100,100) (110,200) (120,300) (130,200) (140,100)
  //---------------------------------------------------------------------------
  TEST_EQUAL(e.size(), 2)
    TEST_EQUAL(e[0].size(), 3)
    TEST_EQUAL(e[1].size(), 5)

    TEST_REAL_SIMILAR(e[0][0].getPosition()[0], 110)
    TEST_REAL_SIMILAR(e[0][0].getIntensity(), 100)
    TEST_REAL_SIMILAR(e[0][1].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e[0][1].getIntensity(), 200)
    TEST_REAL_SIMILAR(e[0][2].getPosition()[0], 130)
    TEST_REAL_SIMILAR(e[0][2].getIntensity(), 100)
    TEST_REAL_SIMILAR(e[1][0].getPosition()[0], 100)
    TEST_REAL_SIMILAR(e[1][0].getIntensity(), 100)
    TEST_REAL_SIMILAR(e[1][1].getPosition()[0], 110)
    TEST_REAL_SIMILAR(e[1][1].getIntensity(), 200)
    TEST_REAL_SIMILAR(e[1][2].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e[1][2].getIntensity(), 300)
    TEST_REAL_SIMILAR(e[1][3].getPosition()[0], 130)
    TEST_REAL_SIMILAR(e[1][3].getIntensity(), 200)
    TEST_REAL_SIMILAR(e[1][4].getPosition()[0], 140)
    TEST_REAL_SIMILAR(e[1][4].getIntensity(), 100)
}
END_SECTION

START_SECTION(([EXTRA] load with intensity range))
{
  TOLERANCE_ABSOLUTE(0.01)

    PeakMap e;
  MzXMLFile file;
  file.getOptions().setIntensityRange(makeRange(150, 350));
  file.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"),e);
  //---------------------------------------------------------------------------
  // 60 : -(120,100)
  // 120: -(110,100) +(120,200) -(130,100)
  // 180: -(100,100) +(110,200) +(120,300) +(130,200) -(140,100)
  //---------------------------------------------------------------------------
  TEST_EQUAL(e[0].size(), 0)
    TEST_EQUAL(e[1].size(), 1)
    TEST_EQUAL(e[2].size(), 3)

    TEST_REAL_SIMILAR(e[1][0].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e[1][0].getIntensity(), 200)
    TEST_REAL_SIMILAR(e[2][0].getPosition()[0], 110)
    TEST_REAL_SIMILAR(e[2][0].getIntensity(), 200)
    TEST_REAL_SIMILAR(e[2][1].getPosition()[0], 120)
    TEST_REAL_SIMILAR(e[2][1].getIntensity(), 300)
    TEST_REAL_SIMILAR(e[2][2].getPosition()[0], 130)
    TEST_REAL_SIMILAR(e[2][2].getIntensity(), 200)
}
END_SECTION

START_SECTION(([EXTRA] load/store for nested scans))
{
  std::string tmp_filename;
  NEW_TMP_FILE(tmp_filename);
  MzXMLFile f;
  PeakMap e2;
  e2.resize(5);

  //alternating
  e2[0].setMSLevel(1);
  e2[1].setMSLevel(2);
  e2[2].setMSLevel(1);
  e2[3].setMSLevel(2);
  e2[4].setMSLevel(1);
  f.store(tmp_filename,e2);
  f.load(tmp_filename,e2);
  TEST_EQUAL(e2.size(),5);

  //ending with ms level 2
  e2[0].setMSLevel(1);
  e2[1].setMSLevel(2);
  e2[2].setMSLevel(1);
  e2[3].setMSLevel(2);
  e2[4].setMSLevel(2);
  f.store(tmp_filename,e2);
  f.load(tmp_filename,e2);
  TEST_EQUAL(e2.size(),5);

  //MS level 1-3
  e2[0].setMSLevel(1);
  e2[1].setMSLevel(2);
  e2[2].setMSLevel(3);
  e2[3].setMSLevel(2);
  e2[4].setMSLevel(3);
  f.store(tmp_filename,e2);
  f.load(tmp_filename,e2);
  TEST_EQUAL(e2.size(),5);

  //MS level 2
  e2[0].setMSLevel(2);
  e2[1].setMSLevel(2);
  e2[2].setMSLevel(2);
  e2[3].setMSLevel(2);
  e2[4].setMSLevel(2);
  f.store(tmp_filename,e2);
  f.load(tmp_filename,e2);
  TEST_EQUAL(e2.size(),5);

  //MS level 2-3
  e2[0].setMSLevel(2);
  e2[1].setMSLevel(2);
  e2[2].setMSLevel(3);
  e2[3].setMSLevel(2);
  e2[4].setMSLevel(3);
  f.store(tmp_filename,e2);
  f.load(tmp_filename,e2);
  TEST_EQUAL(e2.size(),5);

  //MS level 1-3 (not starting with 1)
  e2[0].setMSLevel(2);
  e2[1].setMSLevel(1);
  e2[2].setMSLevel(2);
  e2[3].setMSLevel(3);
  e2[4].setMSLevel(1);
  f.store(tmp_filename,e2);
  f.load(tmp_filename,e2);
  TEST_EQUAL(e2.size(),5);
}
END_SECTION

START_SECTION((template<typename MapType> void store(const std::string& filename, const MapType& map) const ))
{
  std::string tmp_filename;
  PeakMap e1, e2;
  MzXMLFile f;

  NEW_TMP_FILE(tmp_filename);
  f.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"),e1);
  TEST_EQUAL(e1.size(), 4)

    f.store(tmp_filename, e1);
  f.load(tmp_filename, e2);
  TEST_TRUE(e1 == e2);
}
END_SECTION

START_SECTION([EXTRA] static bool isValid(const std::string& filename))
{
  std::string tmp_filename;
  MzXMLFile f;
  PeakMap e;

  //Note: empty mzXML files are not valid, thus this test is omitted

  // test if full file is valid
  NEW_TMP_FILE(tmp_filename);
  f.load(OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML"), e);
  f.store(tmp_filename, e);
  TEST_EQUAL(f.isValid(tmp_filename, std::cerr), true);
}
END_SECTION

START_SECTION(void transform(const std::string& filename_in, Interfaces::IMSDataConsumer * consumer, bool skip_full_count = false))
{
  // Create the consumer, set output file name, transform
  TICConsumer consumer;
  MzXMLFile f;
  std::string in = OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML");

  PeakFileOptions opt = f.getOptions();
  opt.setFillData(true); // whether to actually load any data
  opt.setSkipXMLChecks(true); // save time by not checking base64 strings for whitespaces 
  opt.setMaxDataPoolSize(100);
  opt.setAlwaysAppendData(false);
  f.setOptions(opt);
  f.transform(in, &consumer, true);

  TEST_EQUAL(consumer.nr_spectra, 4)
  TEST_EQUAL(consumer.nr_peaks, 14)
  TEST_REAL_SIMILAR(consumer.TIC, 2300)
}
END_SECTION

START_SECTION(void transform(const std::string& filename_in, Interfaces::IMSDataConsumer * consumer, MapType& map, bool skip_full_count = false) )
{
  // Create the consumer, set output file name, transform
  TICConsumer consumer;
  MzXMLFile f;
  PeakMap map;
  std::string in = OPENMS_GET_TEST_DATA_PATH("MzXMLFile_1.mzXML");

  PeakFileOptions opt = f.getOptions();
  opt.setFillData(true); // whether to actually load any data
  opt.setSkipXMLChecks(true); // save time by not checking base64 strings for whitespaces 
  opt.setMaxDataPoolSize(100);
  opt.setAlwaysAppendData(false);
  f.setOptions(opt);
  f.transform(in, &consumer, map, true);

  TEST_EQUAL(consumer.nr_spectra, 4)
  TEST_EQUAL(consumer.nr_peaks, 14)
  TEST_REAL_SIMILAR(consumer.TIC, 2300)

  TEST_EQUAL(map.getNrSpectra(), 4)
}
END_SECTION

/// Collects what OPENMS_LOG_WARN writes while it exists, on this thread and on the OpenMP threads that decode the
/// scans. OPENMS_LOG_WARN writes to a per-thread stream that copies the sinks of the global warning stream when the
/// thread first logs, so the capture is added to the global stream (for threads that have not logged yet) and to
/// the stream of each thread of a parallel region (for threads that have).
class WarningCapture
{
public:
  WarningCapture()
  {
    getGlobalLogWarn().insert(text_);
#ifdef _OPENMP
    #pragma omp parallel
#endif
    getThreadLocalLogWarn().insert(text_);
  }

  ~WarningCapture()
  {
#ifdef _OPENMP
    #pragma omp parallel
#endif
    getThreadLocalLogWarn().remove(text_);
    getGlobalLogWarn().remove(text_);
  }

  /// Returns the lines written since the last call, sorted (threads log in any order) and each without its
  /// colour codes (written to any stream other than a console) and its "While loading '<file>': " prefix.
  /// A line without that exact prefix is kept whole.
  std::string take(const std::string& file)
  {
    const std::string loading = "While loading '" + file + "': ";
    const std::regex colour("\x1b\\[[0-9;]*m");
    std::istringstream written(text_.str());
    text_.str("");
    std::vector<std::string> lines;
    for (std::string line; std::getline(written, line);)
    {
      line = std::regex_replace(line, colour, "");
      lines.push_back(line.compare(0, loading.size(), loading) == 0 ? line.substr(loading.size()) : line);
    }
    std::sort(lines.begin(), lines.end());
    std::string joined;
    for (const std::string& line : lines)
    {
      joined += line + "\n";
    }
    return joined;
  }

private:
  std::ostringstream text_;
};

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
START_SECTION((regression : SAX chunk boundaries and mismatched peak counts))
{
  MzXMLFile file;
  std::string input;
  NEW_TMP_FILE(input)
  const std::string prefix = "<?xml version=\"1.0\"?><mzXML><msRun scanCount=\"1\">"
                             "<msInstrument><comment>Instr<!-- split -->ument <![CDATA[Comment]]></comment></msInstrument>";
  const std::string suffix = "</scan></msRun></mzXML>";
  std::ofstream(input) << prefix
                       << "<scan num=\"1\" msLevel=\"2\" peaksCount=\"1\" retentionTime=\"PT1S\">"
                          "<precursorMz precursorIntensity=\"5\" windowWideness=\"10\">12<!-- split -->3.<![CDATA[45]]></precursorMz>"
                          "<peaks precision=\"32\" byteOrder=\"network\" contentType=\"m/z-int\">QvAAAELIAAA=</peaks>"
                          "<comment>Scan<!-- split --> Comment</comment>"
                       << suffix;
  PeakMap result;
  file.load(input, result);
  TEST_EQUAL(result.size(), 1)
  ABORT_IF(result.size() != 1)
  TEST_EQUAL(result[0].getPrecursors().size(), 1)
  ABORT_IF(result[0].getPrecursors().size() != 1)
  TEST_REAL_SIMILAR(result[0].getPrecursors()[0].getMZ(), 123.45)
  TEST_REAL_SIMILAR(result[0].getPrecursors()[0].getIsolationWindowLowerOffset(), 5.0)
  TEST_REAL_SIMILAR(result[0].getPrecursors()[0].getIsolationWindowUpperOffset(), 5.0)
  TEST_STRING_EQUAL(result[0].getComment(), "Scan Comment")
  TEST_STRING_EQUAL(result.getInstrument().getMetaValue("#comment").toString(), "Instrument Comment")

  // A peaksCount that disagrees with the decoded payload does not fail the file: each such scan keeps the
  // pairs that are both declared and decoded (all decoded pairs for a count that is negative or outside the
  // Int range), and one warning names the file, the scan, the declared text and the decoded length. Every
  // case has its own scan numbers, so LogStream's suppression of a repeated line cannot hide a warning.
  WarningCapture warnings;
  const std::string one_pair = "QvAAAELIAAA=";                     // (120, 100)
  const std::string two_pairs = "QvAAAELIAABDAgAAQ0gAAA==";        // (120, 100), (130, 200)
  const std::string three_values = "QvAAAELIAABDAgAA";             // (120, 100), 130
  const std::string two_pairs_64 = "QF4AAAAAAABAWQAAAAAAAEBgQAAAAAAAQGkAAAAAAAA="; // (120, 100), (130, 200)
  auto scan = [](const std::string& num, const std::string& count, const std::string& precision, const std::string& payload) {
    return "<scan num=\"" + num + "\" msLevel=\"1\" peaksCount=\"" + count + "\" retentionTime=\"PT" + num
           + "S\"><peaks precision=\"" + precision + "\" byteOrder=\"network\" contentType=\"m/z-int\">" + payload + "</peaks></scan>";
  };
  std::string warned; // the warnings of the last loadScans, see WarningCapture::take
  auto loadScans = [&](const std::string& scans) {
    std::string malformed;
    NEW_TMP_FILE(malformed)
    std::ofstream(malformed) << prefix << scans << "</msRun></mzXML>";
    PeakMap loaded;
    warnings.take(malformed);
    file.load(malformed, loaded);
    warned = warnings.take(malformed);
    File::remove(malformed);
    return loaded;
  };

  // declared larger than decoded: only the decoded pair is read, nothing past the payload
  PeakMap larger = loadScans(scan("11", "2", "32", one_pair));
  TEST_STRING_EQUAL(warned, "Scan 'scan=11' declares peaksCount=\"2\", but its peaks decode to 2 values (1 m/z-intensity pairs). Reading 1 pairs.\n")
  TEST_EQUAL(larger.size(), 1)
  ABORT_IF(larger.size() != 1)
  TEST_EQUAL(larger[0].size(), 1)
  ABORT_IF(larger[0].size() != 1)
  TEST_REAL_SIMILAR(larger[0][0].getMZ(), 120.0)
  TEST_REAL_SIMILAR(larger[0][0].getIntensity(), 100.0)

  // the same in 64-bit precision
  PeakMap larger_64 = loadScans(scan("12", "3", "64", two_pairs_64));
  TEST_STRING_EQUAL(warned, "Scan 'scan=12' declares peaksCount=\"3\", but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n")
  TEST_EQUAL(larger_64.size(), 1)
  ABORT_IF(larger_64.size() != 1)
  TEST_EQUAL(larger_64[0].size(), 2)
  ABORT_IF(larger_64[0].size() != 2)
  TEST_REAL_SIMILAR(larger_64[0][1].getMZ(), 130.0)
  TEST_REAL_SIMILAR(larger_64[0][1].getIntensity(), 200.0)

  // an odd number of decoded values: the unpaired trailing m/z is not read together with an intensity past the end
  PeakMap odd = loadScans(scan("13", "2", "32", three_values));
  TEST_STRING_EQUAL(warned, "Scan 'scan=13' declares peaksCount=\"2\", but its peaks decode to 3 values (1 m/z-intensity pairs). Reading 1 pairs.\n")
  TEST_EQUAL(odd.size(), 1)
  ABORT_IF(odd.size() != 1)
  TEST_EQUAL(odd[0].size(), 1)
  ABORT_IF(odd[0].size() != 1)
  TEST_REAL_SIMILAR(odd[0][0].getMZ(), 120.0)

  // declared smaller than decoded: only the declared pairs are read
  PeakMap smaller = loadScans(scan("14", "1", "32", two_pairs) + scan("15", "0", "32", one_pair));
  TEST_STRING_EQUAL(warned, "Scan 'scan=14' declares peaksCount=\"1\", but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 1 pairs.\n"
                            "Scan 'scan=15' declares peaksCount=\"0\", but its peaks decode to 2 values (1 m/z-intensity pairs). Reading 0 pairs.\n")
  TEST_EQUAL(smaller.size(), 2)
  ABORT_IF(smaller.size() != 2)
  TEST_EQUAL(smaller[0].size(), 1)
  ABORT_IF(smaller[0].size() != 1)
  TEST_REAL_SIMILAR(smaller[0][0].getMZ(), 120.0)
  TEST_REAL_SIMILAR(smaller[0][0].getIntensity(), 100.0)
  TEST_EQUAL(smaller[1].size(), 0)

  // negative declared: the decoded length is used (and the reserve stays bounded)
  PeakMap negative = loadScans(scan("16", "-1", "32", two_pairs));
  TEST_STRING_EQUAL(warned, "Scan 'scan=16' declares peaksCount=\"-1\" (not a valid count), but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n")
  TEST_EQUAL(negative.size(), 1)
  ABORT_IF(negative.size() != 1)
  TEST_EQUAL(negative[0].size(), 2)
  ABORT_IF(negative[0].size() != 2)
  TEST_REAL_SIMILAR(negative[0][0].getMZ(), 120.0)
  TEST_REAL_SIMILAR(negative[0][1].getMZ(), 130.0)
  TEST_REAL_SIMILAR(negative[0][1].getIntensity(), 200.0)

  // a bad scan does not affect a consistent scan next to it in the same file, and only the bad scan is reported
  PeakMap mixed = loadScans(scan("17", "2", "32", two_pairs) + scan("18", "5", "32", one_pair) + scan("19", "2", "32", two_pairs));
  TEST_STRING_EQUAL(warned, "Scan 'scan=18' declares peaksCount=\"5\", but its peaks decode to 2 values (1 m/z-intensity pairs). Reading 1 pairs.\n")
  TEST_EQUAL(mixed.size(), 3)
  ABORT_IF(mixed.size() != 3)
  TEST_EQUAL(mixed[0].size(), 2)
  TEST_EQUAL(mixed[1].size(), 1)
  TEST_EQUAL(mixed[2].size(), 2)
  ABORT_IF(mixed[0].size() != 2 || mixed[1].size() != 1 || mixed[2].size() != 2)
  TEST_STRING_EQUAL(mixed[1].getNativeID(), "scan=18")
  TEST_REAL_SIMILAR(mixed[1][0].getMZ(), 120.0)
  TEST_REAL_SIMILAR(mixed[2][1].getMZ(), 130.0)
  TEST_REAL_SIMILAR(mixed[2][1].getIntensity(), 200.0)

  // consistent scans log nothing: 32 and 64 bit, an empty payload with peaksCount 0, and a count with the
  // surrounding whitespace and leading '+' that the attribute parser accepts
  PeakMap consistent = loadScans(scan("20", "2", "32", two_pairs) + scan("21", "2", "64", two_pairs_64)
                                 + scan("22", "0", "32", "") + scan("23", " +2 ", "32", two_pairs));
  TEST_STRING_EQUAL(warned, "")
  TEST_EQUAL(consistent.size(), 4)
  ABORT_IF(consistent.size() != 4)
  TEST_EQUAL(consistent[0].size(), 2)
  TEST_EQUAL(consistent[1].size(), 2)
  TEST_EQUAL(consistent[2].size(), 0)
  TEST_EQUAL(consistent[3].size(), 2)

  // an empty payload with a nonzero or negative count is reported like a whitespace-only payload
  PeakMap empty = loadScans(scan("24", "5", "32", "") + scan("25", "5", "32", "\n  ") + scan("26", "-1", "32", "")
                            + scan("27", "4294967296", "64", ""));
  TEST_STRING_EQUAL(warned, "Scan 'scan=24' declares peaksCount=\"5\", but its peaks decode to 0 values (0 m/z-intensity pairs). Reading 0 pairs.\n"
                            "Scan 'scan=25' declares peaksCount=\"5\", but its peaks decode to 0 values (0 m/z-intensity pairs). Reading 0 pairs.\n"
                            "Scan 'scan=26' declares peaksCount=\"-1\" (not a valid count), but its peaks decode to 0 values (0 m/z-intensity pairs). Reading 0 pairs.\n"
                            "Scan 'scan=27' declares peaksCount=\"4294967296\" (not a valid count), but its peaks decode to 0 values (0 m/z-intensity pairs). Reading 0 pairs.\n")
  TEST_EQUAL(empty.size(), 4)
  ABORT_IF(empty.size() != 4)
  TEST_EQUAL(empty[0].size() + empty[1].size() + empty[2].size() + empty[3].size(), 0)

  // <peaks> before <precursorMz> (not schema order) is decoded when the precursor starts; its count is checked
  // once there, not a second time against the payload that decoding has already consumed
  auto peaksFirst = [](const std::string& num, const std::string& count, const std::string& payload) {
    return "<scan num=\"" + num + "\" msLevel=\"2\" peaksCount=\"" + count + "\" retentionTime=\"PT" + num
           + "S\"><peaks precision=\"32\" byteOrder=\"network\" contentType=\"m/z-int\">" + payload
           + "</peaks><precursorMz precursorIntensity=\"5\">500.5</precursorMz></scan>";
  };
  PeakMap peaks_first = loadScans(peaksFirst("28", "1", one_pair) + peaksFirst("29", "3", one_pair));
  TEST_STRING_EQUAL(warned, "Scan 'scan=29' declares peaksCount=\"3\", but its peaks decode to 2 values (1 m/z-intensity pairs). Reading 1 pairs.\n")
  TEST_EQUAL(peaks_first.size(), 2)
  ABORT_IF(peaks_first.size() != 2)
  TEST_EQUAL(peaks_first[0].size(), 1)
  TEST_EQUAL(peaks_first[1].size(), 1)
  TEST_EQUAL(peaks_first[0].getPrecursors().size(), 1)
  ABORT_IF(peaks_first[0].getPrecursors().size() != 1)
  TEST_REAL_SIMILAR(peaks_first[0].getPrecursors()[0].getMZ(), 500.5)

  // Only the payload that a <precursorMz> has decoded early is skipped when the scans are populated. A later
  // <peaks> of the same scan is still decoded, and each payload is checked against the count on its own: two
  // payloads (39), the same followed by a second precursor, whose m/z must not be mixed with the second payload
  // (40), and an empty second payload, which has nothing to decode or report (41).
  auto peaks = [](const std::string& payload) {
    return "<peaks precision=\"32\" byteOrder=\"network\" contentType=\"m/z-int\">" + payload + "</peaks>";
  };
  auto precursor = [](const std::string& mz) {
    return "<precursorMz precursorIntensity=\"5\">" + mz + "</precursorMz>";
  };
  auto ms2Scan = [](const std::string& num, const std::string& count, const std::string& content) {
    return "<scan num=\"" + num + "\" msLevel=\"2\" peaksCount=\"" + count + "\" retentionTime=\"PT" + num + "S\">" + content + "</scan>";
  };
  PeakMap peaks_twice = loadScans(ms2Scan("39", "3", peaks(one_pair) + precursor("500.5") + peaks(two_pairs))
                                  + ms2Scan("40", "3", peaks(one_pair) + precursor("500.5") + peaks(two_pairs) + precursor("600.5"))
                                  + ms2Scan("41", "1", peaks(one_pair) + precursor("500.5") + peaks("")));
  TEST_STRING_EQUAL(warned, "Scan 'scan=39' declares peaksCount=\"3\", but its peaks decode to 2 values (1 m/z-intensity pairs). Reading 1 pairs.\n"
                            "Scan 'scan=39' declares peaksCount=\"3\", but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n"
                            "Scan 'scan=40' declares peaksCount=\"3\", but its peaks decode to 2 values (1 m/z-intensity pairs). Reading 1 pairs.\n"
                            "Scan 'scan=40' declares peaksCount=\"3\", but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n")
  TEST_EQUAL(peaks_twice.size(), 3)
  ABORT_IF(peaks_twice.size() != 3)
  TEST_EQUAL(peaks_twice[0].size(), 3)
  TEST_EQUAL(peaks_twice[1].size(), 3)
  TEST_EQUAL(peaks_twice[2].size(), 1)
  ABORT_IF(peaks_twice[0].size() != 3 || peaks_twice[1].size() != 3)
  TEST_REAL_SIMILAR(peaks_twice[0][0].getMZ(), 120.0)
  TEST_REAL_SIMILAR(peaks_twice[0][2].getMZ(), 130.0)
  TEST_REAL_SIMILAR(peaks_twice[0][2].getIntensity(), 200.0)
  TEST_REAL_SIMILAR(peaks_twice[1][2].getMZ(), 130.0)
  TEST_EQUAL(peaks_twice[1].getPrecursors().size(), 2)
  ABORT_IF(peaks_twice[1].getPrecursors().size() != 2)
  TEST_REAL_SIMILAR(peaks_twice[1].getPrecursors()[0].getMZ(), 500.5)
  TEST_REAL_SIMILAR(peaks_twice[1].getPrecursors()[1].getMZ(), 600.5)

  // <peaks> after <precursorMz> (schema order) is decoded when the scans are populated and checked once
  PeakMap precursor_first = loadScans(ms2Scan("42", "2", precursor("500.5") + peaks(two_pairs))
                                      + ms2Scan("43", "3", precursor("500.5") + peaks(two_pairs)));
  TEST_STRING_EQUAL(warned, "Scan 'scan=43' declares peaksCount=\"3\", but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n")
  TEST_EQUAL(precursor_first.size(), 2)
  ABORT_IF(precursor_first.size() != 2)
  TEST_EQUAL(precursor_first[0].size(), 2)
  TEST_EQUAL(precursor_first[1].size(), 2)
  ABORT_IF(precursor_first[1].size() != 2 || precursor_first[1].getPrecursors().size() != 1)
  TEST_REAL_SIMILAR(precursor_first[1][1].getMZ(), 130.0)
  TEST_REAL_SIMILAR(precursor_first[1].getPrecursors()[0].getMZ(), 500.5)

  // a count outside the Int range is not wrapped into a different count: like a negative count, it reads
  // the decoded pairs, and the warning shows the text of the file. INT_MAX and INT_MIN are still counts.
  PeakMap out_of_range = loadScans(scan("30", "4294967296", "32", two_pairs) + scan("31", "4294967295", "32", two_pairs)
                                   + scan("32", "99999999999", "32", two_pairs) + scan("33", "2147483648", "32", two_pairs)
                                   + scan("34", "-2147483649", "32", two_pairs) + scan("35", "2147483647", "32", two_pairs)
                                   + scan("36", "-2147483648", "32", two_pairs));
  TEST_STRING_EQUAL(warned, "Scan 'scan=30' declares peaksCount=\"4294967296\" (not a valid count), but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n"
                            "Scan 'scan=31' declares peaksCount=\"4294967295\" (not a valid count), but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n"
                            "Scan 'scan=32' declares peaksCount=\"99999999999\" (not a valid count), but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n"
                            "Scan 'scan=33' declares peaksCount=\"2147483648\" (not a valid count), but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n"
                            "Scan 'scan=34' declares peaksCount=\"-2147483649\" (not a valid count), but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n"
                            "Scan 'scan=35' declares peaksCount=\"2147483647\", but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n"
                            "Scan 'scan=36' declares peaksCount=\"-2147483648\" (not a valid count), but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n")
  TEST_EQUAL(out_of_range.size(), 7)
  ABORT_IF(out_of_range.size() != 7)
  for (const MSSpectrum& spectrum : out_of_range)
  {
    TEST_EQUAL(spectrum.size(), 2)
  }

  // text that is not an integer still fails the load
  std::string not_a_count;
  NEW_TMP_FILE(not_a_count)
  std::ofstream(not_a_count) << prefix << scan("37", "2x", "32", two_pairs) << "</msRun></mzXML>";
  PeakMap not_loaded;
  TEST_EXCEPTION(Exception::ParseError, file.load(not_a_count, not_loaded))
  File::remove(not_a_count);

  // a count beyond the 64-bit range is outside the Int range too
  PeakMap beyond_64 = loadScans(scan("38", "99999999999999999999", "32", two_pairs));
  TEST_STRING_EQUAL(warned, "Scan 'scan=38' declares peaksCount=\"99999999999999999999\" (not a valid count), but its peaks decode to 4 values (2 m/z-intensity pairs). Reading 2 pairs.\n")
  TEST_EQUAL(beyond_64.size(), 1)
  ABORT_IF(beyond_64.size() != 1)
  TEST_EQUAL(beyond_64[0].size(), 2)

  // Deliberately incomplete reader fixtures are excluded from writer-schema checks.
  File::remove(input);
}
END_SECTION

/// check the temporary files written above against their XML schema (types without a validator are skipped)
VALIDATE_TMP_FILES

END_TEST
