// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg$
// $Authors: Erhan Kenar$
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>
#include <OpenMS/FORMAT/MzMLFile.h>
#include <OpenMS/CONCEPT/Constants.h>

///////////////////////////
#include <OpenMS/FEATUREFINDER/MassTraceDetection.h>
///////////////////////////

using namespace OpenMS;
using namespace std;

/// provide access to private functions
class MassTraceDetectionAccess : public OpenMS::MassTraceDetection
{
public:
  using OpenMS::MassTraceDetection::updateIterativeWeightedMean_;
};

START_TEST(MassTraceDetection, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

MassTraceDetection* ptr = nullptr;
MassTraceDetection* null_ptr = nullptr;
START_SECTION(MassTraceDetection())
{
    ptr = new MassTraceDetection();
    TEST_NOT_EQUAL(ptr, null_ptr)
}
END_SECTION

START_SECTION(~MassTraceDetection())
{
    delete ptr;
}
END_SECTION

MassTraceDetection test_mtd;

START_SECTION((void updateIterativeWeightedMean_(const double &, const double &, double &, double &, double &)))
{
    double centroid_mz(150.22), centroid_int(25000000);
    double new_mz1(150.34), new_int1(23043030);
    double new_mz2(150.11), new_int2(1932392);

    std::vector<double> mzs, ints;
    mzs.push_back(centroid_mz);
    mzs.push_back(new_mz1);
    mzs.push_back(new_mz2);
    ints.push_back(centroid_int);
    ints.push_back(new_int1);
    ints.push_back(new_int2);

    double total_weight1(centroid_int + new_int1);
    double total_weight2(centroid_int + new_int1 + new_int2);

    double wmean1((centroid_mz * centroid_int + new_mz1 * new_int1)/total_weight1);
    double wmean2((centroid_mz * centroid_int + new_mz1 * new_int1 + new_mz2 * new_int2)/total_weight2);

    double prev_count(centroid_mz * centroid_int);
    double prev_denom(centroid_int);

    MassTraceDetectionAccess::updateIterativeWeightedMean_(new_mz1, new_int1, centroid_mz, prev_count, prev_denom);


    TEST_REAL_SIMILAR(centroid_mz, wmean1);

    MassTraceDetectionAccess::updateIterativeWeightedMean_(new_mz2, new_int2, centroid_mz, prev_count, prev_denom);

    TEST_REAL_SIMILAR(centroid_mz, wmean2);

}
END_SECTION

// load a mzML file for testing the algorithm
PeakMap input;
MzMLFile().load(OPENMS_GET_TEST_DATA_PATH("MassTraceDetection_input1.mzML"),input);

Size exp_mt_lengths[3] = {86, 31, 16};
double exp_mt_rts[3] = {348.667, 347.107, 346.888}; // centroid RTs should be reasonably similar (isotopic traces)
double exp_mt_mzs[3] = {437.26675, 438.27241, 439.27594};
double exp_mt_ints[3] = {3381.72226139326, 664.763828332733, 109.490108620676};

std::vector<MassTrace> output_mt;

Param p_mtd = MassTraceDetection().getDefaults();
p_mtd.setValue("min_trace_length", 3.0);

START_SECTION((void run(const PeakMap &, std::vector< MassTrace > &)))
{
    test_mtd.run(input, output_mt);

    // with default parameters, only 2 of 3 traces will be found
    TEST_EQUAL(output_mt.size(), 2);

    // if min_trace_length is set to 3 seconds, another mass trace is detected
    test_mtd.setParameters(p_mtd);
    output_mt.clear();

    test_mtd.run(input, output_mt);

    TEST_EQUAL(output_mt.size(), 3);

    for (Size i = 0; i < output_mt.size(); ++i)
    {
        TEST_EQUAL(output_mt[i].getSize(), exp_mt_lengths[i]);
        TEST_REAL_SIMILAR(output_mt[i].getCentroidRT(), exp_mt_rts[i]);
        TEST_REAL_SIMILAR(output_mt[i].getCentroidMZ(), exp_mt_mzs[i]);
        TEST_REAL_SIMILAR(output_mt[i].computePeakArea(), exp_mt_ints[i]);
    }

    // Regression test for bug #1633
    // Test by adding MS2 spectra to the input
    {
      PeakMap input_new;
      MSSpectrum s;
      s.setMSLevel(2);
      {
        Peak1D p;
        p.setMZ( 500 );
        p.setIntensity( 6000 );
        s.push_back(p);
      }

      // add a few additional MS2 spectra in front
      for (Size i = 0; i < input.size(); ++i)
      {
        input_new.addSpectrum(s);
      }
      // now add the "real" spectra at the end
      for (Size i = 0; i < input.size(); ++i)
      {
        input_new.addSpectrum(input[i]);
      }
      output_mt.clear();
      test_mtd.run(input_new, output_mt);
      TEST_EQUAL(output_mt.size(), 3);

      for (Size i = 0; i < output_mt.size(); ++i)
      {
          TEST_EQUAL(output_mt[i].getSize(), exp_mt_lengths[i]);
          TEST_REAL_SIMILAR(output_mt[i].getCentroidRT(), exp_mt_rts[i]);
          TEST_REAL_SIMILAR(output_mt[i].getCentroidMZ(), exp_mt_mzs[i]);
          TEST_REAL_SIMILAR(output_mt[i].computePeakArea(), exp_mt_ints[i]);
      }

    }
}
END_SECTION

START_SECTION(([EXTRA] void run(const PeakMap &, std::vector< MassTrace > &) reusing one detector for inputs with different float data arrays))
{
  // Regression test: the float data array flags and indices found by one run must not leak into
  // the next run of the same detector. Every run has to give the result of a fresh detector.

  // copy of the input with float data arrays added in the given order; the values of all arrays
  // except the (constant) ion mobility change from spectrum to spectrum by far more than
  // ion_mobility_tolerance, so reading any of them as ion mobility breaks every mass trace
  auto withArrays = [&input](const std::vector<std::string>& names)
  {
    PeakMap result = input;
    for (Size s = 0; s < result.size(); ++s)
    {
      MSSpectrum& spec = result[s];
      MSSpectrum::FloatDataArrays fdas;
      for (const std::string& name : names)
      {
        MSSpectrum::FloatDataArray fda;
        fda.setName(name);
        float value = 0.9f;
        if (name == Constants::UserParam::FWHM_MZ_ppm) value = 4.0f + 2.0f * (s % 2);
        if (name == Constants::UserParam::FWHM_IM) value = 0.05f + 1.0f * (s % 2);
        fda.assign(spec.size(), value);
        fdas.push_back(fda);
      }
      spec.setFloatDataArrays(fdas);
    }
    return result;
  };

  const PeakMap input_im = withArrays({Constants::UserParam::ION_MOBILITY, Constants::UserParam::FWHM_MZ_ppm, Constants::UserParam::FWHM_IM});
  const PeakMap input_reordered = withArrays({Constants::UserParam::FWHM_IM, Constants::UserParam::ION_MOBILITY, Constants::UserParam::FWHM_MZ_ppm});
  const PeakMap input_reordered_no_im = withArrays({Constants::UserParam::FWHM_MZ_ppm, Constants::UserParam::FWHM_IM});
  const PeakMap input_no_fwhm_mz = withArrays({Constants::UserParam::ION_MOBILITY, Constants::UserParam::FWHM_IM});
  const PeakMap input_no_fwhm_im = withArrays({Constants::UserParam::FWHM_MZ_ppm, Constants::UserParam::ION_MOBILITY});
  const PeakMap& input_no_arrays = input;

  MassTraceDetection reused;
  reused.setParameters(p_mtd);

  auto runAndCompareWithFresh = [&reused, &p_mtd](const std::string& label, const PeakMap& exp, bool expect_im, bool expect_fwhm_mz, bool expect_fwhm_im)
  {
    // all checks share the lines below, so name the input; flush so the name survives a crash
    STATUS("reused detector, input: " << label)
    std::cout.flush();
    MassTraceDetection fresh;
    fresh.setParameters(p_mtd);
    std::vector<MassTrace> expected, observed;
    fresh.run(exp, expected);
    reused.run(exp, observed);

    TEST_EQUAL(fresh.hasCentroidIm(), expect_im)
    TEST_EQUAL(fresh.hasFwhmMz(), expect_fwhm_mz)
    TEST_EQUAL(fresh.hasFwhmIm(), expect_fwhm_im)
    TEST_EQUAL(reused.hasCentroidIm(), fresh.hasCentroidIm())
    TEST_EQUAL(reused.hasFwhmMz(), fresh.hasFwhmMz())
    TEST_EQUAL(reused.hasFwhmIm(), fresh.hasFwhmIm())

    TEST_EQUAL(expected.size(), 3)
    TEST_EQUAL(observed.size(), expected.size())
    for (Size i = 0; i < std::min(observed.size(), expected.size()); ++i)
    {
      TEST_EQUAL(observed[i].getSize(), expected[i].getSize())
      TEST_REAL_SIMILAR(observed[i].getCentroidRT(), expected[i].getCentroidRT())
      TEST_REAL_SIMILAR(observed[i].getCentroidMZ(), expected[i].getCentroidMZ())
      TEST_REAL_SIMILAR(observed[i].computePeakArea(), expected[i].computePeakArea())
      TEST_EQUAL(observed[i].containsIMData(), expected[i].containsIMData())
      TEST_REAL_SIMILAR(observed[i].getCentroidIM(), expected[i].getCentroidIM())
      TEST_REAL_SIMILAR(observed[i].fwhm_mz_avg, expected[i].fwhm_mz_avg)
      TEST_REAL_SIMILAR(observed[i].fwhm_im_avg, expected[i].fwhm_im_avg)
    }
  };

  // The comments give the array indices a detector that keeps the previous run's indices would use
  // (IM = ion mobility, FWHM_mz = m/z FWHM, FWHM_IM = ion mobility FWHM).
  // IM 0, FWHM_mz 1, FWHM_IM 2
  runAndCompareWithFresh("IM, FWHM_mz, FWHM_IM", input_im, true, true, true);
  // IM 1, FWHM_mz 2, FWHM_IM 0: every index is found again, so this run is a control that
  // passes without the reset too; it moves the indices for the next run
  runAndCompareWithFresh("FWHM_IM, IM, FWHM_mz (reordered)", input_reordered, true, true, true);
  // FWHM_mz 0, FWHM_IM 1; the ion mobility index stays 1 and would read FWHM_IM as ion mobility
  runAndCompareWithFresh("FWHM_mz, FWHM_IM (reordered, no IM)", input_reordered_no_im, false, true, true);
  // IM 0, FWHM_IM 1; the m/z FWHM index stays 0 and would read the IM array as m/z FWHM
  runAndCompareWithFresh("IM, FWHM_IM (reordered, no FWHM_mz)", input_no_fwhm_mz, true, false, true);
  // FWHM_mz 0, IM 1; the ion mobility FWHM index stays 1 and would read the IM array as ion mobility FWHM
  runAndCompareWithFresh("FWHM_mz, IM (reordered, no FWHM_IM)", input_no_fwhm_im, true, true, false);
  // every previous index would point past the end of the (empty) float data arrays
  runAndCompareWithFresh("no float data arrays", input_no_arrays, false, false, false);
  // and back to the first input
  runAndCompareWithFresh("IM, FWHM_mz, FWHM_IM (again)", input_im, true, true, true);
}
END_SECTION

std::vector<MassTrace> filt;

//START_SECTION((void filterByPeakWidth(std::vector< MassTrace > &, std::vector< MassTrace > &)))
//{
//    test_mtd.filterByPeakWidth(output_mt, filt);

//    TEST_EQUAL(output_mt.size(), filt.size());

////    for (Size i = 0; i < output_mt.size(); ++i)
////    {
////        TEST_EQUAL(output_mt[i].getFWHMScansNum(), filt[i].getFWHMScansNum());
////    }
//}
//END_SECTION

PeakMap::ConstAreaIterator mt_it1 = input.areaBeginConst(335.0, 385.0, 437.1, 437.4);
PeakMap::ConstAreaIterator mt_it2 = input.areaBeginConst(335.0, 385.0, 438.2, 438.4);
PeakMap::ConstAreaIterator mt_it3 = input.areaBeginConst(335.0, 385.0, 439.2, 439.4);

std::vector<MassTrace> found_mtraces;

PeakMap::ConstAreaIterator mt_end = input.areaEndConst();

START_SECTION((void run(PeakMap::ConstAreaIterator &begin, PeakMap::ConstAreaIterator &end, std::vector< MassTrace > &found_masstraces)))
{

    NOT_TESTABLE
//    test_mtd.run(mt_it1, mt_end, found_mtraces);
//    TEST_EQUAL(found_mtraces.size(), 1);
//    TEST_EQUAL(found_mtraces[0].getSize(), exp_mt_lengths[0]);

//    TEST_REAL_SIMILAR(found_mtraces[0].getCentroidRT(), exp_mt_rts[0]);
//    TEST_REAL_SIMILAR(found_mtraces[0].getCentroidMZ(), exp_mt_mzs[0]);
//    TEST_REAL_SIMILAR(found_mtraces[0].computePeakArea(), exp_mt_ints[0]);

//    found_mtraces.clear();


//    test_mtd.run(mt_it2, mt_end, found_mtraces);
//    TEST_EQUAL(found_mtraces.size(), 1);
//    TEST_EQUAL(found_mtraces[0].getSize(), exp_mt_lengths[1]);

//    TEST_REAL_SIMILAR(found_mtraces[0].getCentroidRT(), exp_mt_rts[1]);
//    TEST_REAL_SIMILAR(found_mtraces[0].getCentroidMZ(), exp_mt_mzs[1]);
//    TEST_REAL_SIMILAR(found_mtraces[0].computePeakArea(), exp_mt_ints[1]);

//    found_mtraces.clear();


//    test_mtd.run(mt_it3, mt_end, found_mtraces);
//    TEST_EQUAL(found_mtraces.size(), 1);
//    TEST_EQUAL(found_mtraces[0].getSize(), exp_mt_lengths[0]);

//    TEST_REAL_SIMILAR(found_mtraces[0].getCentroidRT(), exp_mt_rts[2]);
//    TEST_REAL_SIMILAR(found_mtraces[0].getCentroidMZ(), exp_mt_mzs[2]);
//    TEST_REAL_SIMILAR(found_mtraces[0].computePeakArea(), exp_mt_ints[2]);

//    found_mtraces.clear();
}
END_SECTION

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST
