// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// 
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>
#include <OpenMS/KERNEL/StandardTypes.h>

///////////////////////////
#include <OpenMS/KERNEL/ConversionHelper.h>
///////////////////////////

using namespace OpenMS;
using namespace std;

START_TEST(ConsensusMap, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

START_SECTION((template < typename FeatureT > static void convert(UInt64 const input_map_index, FeatureMap< FeatureT > const &input_map, ConsensusMap &output_map, Size n=-1)))
{

  FeatureMap fm;
  Feature f;
  for ( UInt i = 0; i < 3; ++i )
  {
    f.setRT(i*77.7);
    f.setMZ(i+100.35);
    f.setUniqueId(i*33+17);
    fm.push_back(f);
  }
  ConsensusMap cm;
  MapConversion::convert(33,fm,cm);

  TEST_EQUAL(cm.size(),3);
  TEST_EQUAL(cm.getColumnHeaders()[33].size,3);
  for ( UInt i = 0; i < 3; ++i )
  {
    TEST_EQUAL(cm[i].size(),1);
    TEST_EQUAL(cm[i].begin()->getMapIndex(),33);
    TEST_EQUAL(cm[i].begin()->getUniqueId(),i*33+17);
    TEST_REAL_SIMILAR(cm[i].begin()->getRT(),i*77.7);
    TEST_REAL_SIMILAR(cm[i].begin()->getMZ(),i+100.35);
  }

cm.clear();
MapConversion::convert(33,fm,cm,2);
TEST_EQUAL(cm.size(),2);
TEST_EQUAL(cm.getColumnHeaders()[33].size,3);

}
END_SECTION

/////

// Prepare data
PeakMap mse;
{
  MSSpectrum mss;
  Peak1D p;
  for ( UInt m = 0; m < 3; ++m )
  {
    mss.clear(true);
    for ( UInt i = 0; i < 4; ++i )
    {
      p.setMZ( 10* m + i + 100.35);
      p.setIntensity( 900 + 7*m + 5*i );
      mss.push_back(p);
    }
    mse.addSpectrum(mss);
    mse.getSpectra().back().setRT(m*5);
  }
}

START_SECTION((static void convert(UInt64 const input_map_index, PeakMap & input_map, ConsensusMap& output_map, Size n = -1)))
{

  ConsensusMap cm;

  MapConversion::convert(33,mse,cm,8);

  TEST_EQUAL(cm.size(),8);

  for ( UInt i = 0; i < cm.size(); ++i)
  {
    STATUS("\n" << i << ": " << cm[i] );
  }

  TEST_EQUAL(cm.back().getIntensity(),912);

}
END_SECTION

START_SECTION(([EXTRA] static void convert(UInt64 const input_map_index, PeakMap & input_map, ConsensusMap& output_map, Size n = -1) with MS2 spectra and chromatograms))
{
  // getSize() also counts MS2 peaks and chromatogram points, but only MS1 peaks are converted,
  // so n must be capped by the number of MS1 peaks
  PeakMap exp;
  Peak1D p;
  MSSpectrum ms1;
  ms1.setMSLevel(1);
  ms1.setRT(5.0);
  const double ms1_intensities[] = { 10.0, 40.0, 20.0, 30.0 };
  for (Size i = 0; i < 4; ++i)
  {
    p.setMZ(100.0 + i);
    p.setIntensity(ms1_intensities[i]);
    ms1.push_back(p);
  }
  exp.addSpectrum(ms1);

  // more intense than every MS1 peak: they must neither be converted nor displace MS1 peaks
  MSSpectrum ms2;
  ms2.setMSLevel(2);
  ms2.setRT(6.0);
  for (Size i = 0; i < 3; ++i)
  {
    p.setMZ(200.0 + i);
    p.setIntensity(1000.0 * (i + 1));
    ms2.push_back(p);
  }
  exp.addSpectrum(ms2);

  MSChromatogram chrom;
  ChromatogramPeak cp;
  for (Size i = 0; i < 2; ++i)
  {
    cp.setRT(1.0 + i);
    cp.setIntensity(5000.0);
    chrom.push_back(cp);
  }
  exp.addChromatogram(chrom);

  exp.updateRanges();
  TEST_EQUAL(exp.getSize(), 9)

  // most intense first
  const double expected_intensities[] = { 40.0, 30.0, 20.0, 10.0 };
  const double expected_mzs[] = { 101.0, 103.0, 102.0, 100.0 };

  ConsensusMap cm;
  MapConversion::convert(7, exp, cm); // default n
  TEST_EQUAL(cm.size(), 4)
  TEST_EQUAL(cm.getColumnHeaders()[7].size, 4)
  for (Size i = 0; i < std::min(cm.size(), Size(4)); ++i)
  {
    TEST_EQUAL(cm[i].size(), 1)
    TEST_EQUAL(cm[i].begin()->getMapIndex(), 7)
    TEST_EQUAL(cm[i].begin()->getUniqueId(), i)
    TEST_REAL_SIMILAR(cm[i].getRT(), 5.0)
    TEST_REAL_SIMILAR(cm[i].getMZ(), expected_mzs[i])
    TEST_REAL_SIMILAR(cm[i].getIntensity(), expected_intensities[i])
  }

  // n larger than the number of MS1 peaks, but not larger than getSize()
  MapConversion::convert(7, exp, cm, 8);
  TEST_EQUAL(cm.size(), 4)
  TEST_EQUAL(cm.getColumnHeaders()[7].size, 4)
  for (Size i = 0; i < std::min(cm.size(), Size(4)); ++i)
  {
    TEST_REAL_SIMILAR(cm[i].getMZ(), expected_mzs[i])
    TEST_REAL_SIMILAR(cm[i].getIntensity(), expected_intensities[i])
  }

  // n smaller than the number of MS1 peaks
  MapConversion::convert(7, exp, cm, 2);
  TEST_EQUAL(cm.size(), 2)
  TEST_EQUAL(cm.getColumnHeaders()[7].size, 2)
  for (Size i = 0; i < std::min(cm.size(), Size(2)); ++i)
  {
    TEST_REAL_SIMILAR(cm[i].getMZ(), expected_mzs[i])
    TEST_REAL_SIMILAR(cm[i].getIntensity(), expected_intensities[i])
  }

  // no MS1 spectrum at all: nothing to convert
  PeakMap ms2_only;
  ms2_only.addSpectrum(ms2);
  ms2_only.addChromatogram(chrom);
  MapConversion::convert(7, ms2_only, cm);
  TEST_EQUAL(cm.size(), 0)
  TEST_EQUAL(cm.getColumnHeaders()[7].size, 0)
}
END_SECTION

/////

ConsensusMap cm;
MapConversion::convert(33,mse,cm,8);

START_SECTION((template < typename FeatureT > static void convert(ConsensusMap const &input_map, const bool keep_uids, FeatureMap< FeatureT > &output_map)))
{
    FeatureMap out_fm;
    MapConversion::convert(cm, true, out_fm);

    TEST_EQUAL(cm.getUniqueId(), out_fm.getUniqueId());
    TEST_EQUAL(cm.getProteinIdentifications().size(), out_fm.getProteinIdentifications().size());
    TEST_EQUAL(cm.getUnassignedPeptideIdentifications().size(), out_fm.getUnassignedPeptideIdentifications().size());
    TEST_EQUAL(cm.size(), out_fm.size());

    for (Size i = 0; i < cm.size(); ++i)
    {
        TEST_EQUAL(cm[i], out_fm[i]);
    }

    out_fm.clear();
    MapConversion::convert(cm, false, out_fm);
    TEST_NOT_EQUAL(cm.getUniqueId(), out_fm.getUniqueId());

    for (Size i = 0; i < cm.size(); ++i)
    {
        TEST_REAL_SIMILAR(cm[i].getRT(), out_fm[i].getRT());
        TEST_REAL_SIMILAR(cm[i].getMZ(), out_fm[i].getMZ());
        TEST_REAL_SIMILAR(cm[i].getIntensity(), out_fm[i].getIntensity());

        TEST_NOT_EQUAL(cm[i].getUniqueId(), out_fm[i].getUniqueId());
    }
}
END_SECTION

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST



