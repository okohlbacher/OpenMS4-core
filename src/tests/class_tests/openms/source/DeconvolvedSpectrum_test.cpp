// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Jihyung Kim$
// $Authors: Jihyung Kim$
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>

///////////////////////////
#include <OpenMS/ANALYSIS/TOPDOWN/DeconvolvedSpectrum.h>
///////////////////////////

using namespace OpenMS;
using namespace std;

START_TEST(DeconvolvedSpectrum, "$Id$")

// Data-container tests construct records without requiring the FLASH algorithm.
MSSpectrum test_spec;
test_spec.setMSLevel(1);
test_spec.setRT(251.72280736002);
test_spec.emplace_back(500.0, 100.0);
DeconvolvedSpectrum prec_deconv_spec_1(2);
prec_deconv_spec_1.setOriginalSpectrum(test_spec);
PeakGroup peak_group(5, 20, true);
peak_group.setMonoisotopicMass(1000.0);
peak_group.push_back(FLASHHelperClasses::LogMzPeak(test_spec[0], true));
prec_deconv_spec_1.push_back(peak_group);

MSSpectrum ms2_spec;
ms2_spec.setMSLevel(2);
DeconvolvedSpectrum ms2_deconv_spec(6);
ms2_deconv_spec.setOriginalSpectrum(ms2_spec);
Precursor precursor;
precursor.setCharge(9);
precursor.setMZ(13682.3053614085 / 9 + Constants::PROTON_MASS_U);
precursor.setIntensity(12293.4);
ms2_deconv_spec.setPrecursor(precursor);
ms2_deconv_spec.setActivationMethod(Precursor::ETD);

DeconvolvedSpectrum test_deconv_spec(1);
test_deconv_spec.setOriginalSpectrum(test_spec);


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

DeconvolvedSpectrum* ptr = 0;
DeconvolvedSpectrum* null_ptr = 0;
START_SECTION(DeconvolvedSpectrum())
  ptr = new DeconvolvedSpectrum();
  TEST_NOT_EQUAL(ptr, null_ptr)
END_SECTION

START_SECTION(~DeconvolvedSpectrum())
  delete ptr;
END_SECTION

START_SECTION(DeconvolvedSpectrum(const DeconvolvedSpectrum&))
  DeconvolvedSpectrum original_spec(1);
  original_spec.setOriginalSpectrum(test_spec);
  DeconvolvedSpectrum copied_spec(original_spec);
  TEST_EQUAL(copied_spec.getScanNumber(), original_spec.getScanNumber());
  TEST_EQUAL(copied_spec.getOriginalSpectrum().size(), original_spec.getOriginalSpectrum().size());
END_SECTION

START_SECTION(DeconvolvedSpectrum&& other)
  DeconvolvedSpectrum original_spec(1);
  original_spec.setOriginalSpectrum(test_spec);
  DeconvolvedSpectrum moved_spec(std::move(original_spec));
  TEST_EQUAL(moved_spec.getScanNumber(), 1);
  TEST_EQUAL(moved_spec.getOriginalSpectrum().size(), test_spec.size());
END_SECTION

START_SECTION(=(const DeconvolvedSpectrum& deconvolved_spectrum))
  DeconvolvedSpectrum spec1(1);
  spec1.setOriginalSpectrum(test_spec);
  DeconvolvedSpectrum spec2;
  spec2 = spec1;
  TEST_EQUAL(spec2.getScanNumber(), spec1.getScanNumber());
  TEST_EQUAL(spec2.getOriginalSpectrum().size(), spec1.getOriginalSpectrum().size());
END_SECTION

START_SECTION((MSSpectrum toSpectrum(const int mass_charge)))
{
  MSSpectrum peakgroup_spec = prec_deconv_spec_1.toSpectrum(9, 1);
  TEST_EQUAL(peakgroup_spec.size(), 1);
  TEST_REAL_SIMILAR(peakgroup_spec.getRT(), 251.72280736002);

  // empty (default-constructed) DeconvolvedSpectrum must not dereference peak_groups_[0] (regression): returns a header-only spectrum
  DeconvolvedSpectrum empty_dspec;
  MSSpectrum empty_out = empty_dspec.toSpectrum(1, 1);
  TEST_EQUAL(empty_out.empty(), true);
}
END_SECTION

START_SECTION((const MSSpectrum& getOriginalSpectrum() const))
{
  MSSpectrum tmp_s = test_deconv_spec.getOriginalSpectrum();
  TEST_EQUAL(tmp_s.size(), test_spec.size());
}
END_SECTION



/// detailed constructor
START_SECTION((DeconvolvedSpectrum(const MSSpectrum &spectrum, const int scan_number)))
{
  DeconvolvedSpectrum tmp_spec = DeconvolvedSpectrum(1);
  tmp_spec.setOriginalSpectrum(test_spec);
  TEST_EQUAL(tmp_spec.getScanNumber(), 1);
  TEST_EQUAL(tmp_spec.getOriginalSpectrum().size(), test_spec.size());
}
END_SECTION


////////
START_SECTION((int getScanNumber() const))
{
  test_deconv_spec.setOriginalSpectrum(test_spec);
  int tmp_num = test_deconv_spec.getScanNumber();
  TEST_EQUAL(tmp_num, 1);
}
END_SECTION






START_SECTION((double getCurrentMaxMass(const double max_mass) const))
{
  double ms1_max_mass = test_deconv_spec.getCurrentMaxMass(1000.);
  double ms2_max_mass = ms2_deconv_spec.getCurrentMaxMass(13673.239337872);
  TOLERANCE_ABSOLUTE(1);
  TEST_REAL_SIMILAR(ms1_max_mass, 1000.);
  TEST_REAL_SIMILAR(ms2_max_mass, 13674.);
}
END_SECTION

START_SECTION((double getCurrentMinMass(const double min_mass) const))
{
  double ms1_min_mass = test_deconv_spec.getCurrentMinMass(1000.);
  double ms2_min_mass = ms2_deconv_spec.getCurrentMinMass(1000.);
  TEST_REAL_SIMILAR(ms1_min_mass, 1000.);
  TEST_REAL_SIMILAR(ms2_min_mass, 50.);
}
END_SECTION



START_SECTION((PeakGroup getPrecursorPeakGroup() const))
{
  PeakGroup tmp_precursor_pgs = ms2_deconv_spec.getPrecursorPeakGroup();

  TEST_EQUAL(tmp_precursor_pgs.size(), 0);
  TOLERANCE_ABSOLUTE(5);
  TEST_REAL_SIMILAR(tmp_precursor_pgs.getMonoMass(), -1);
  TEST_REAL_SIMILAR(tmp_precursor_pgs.getIntensity(), 0);
  TEST_EQUAL(tmp_precursor_pgs.getScanNumber(), 0);
}
END_SECTION

START_SECTION((const Precursor getPrecursor() const))
{
  Precursor tmp_precursor = ms2_deconv_spec.getPrecursor();
  TEST_EQUAL(tmp_precursor.getCharge(), 9);
  TOLERANCE_ABSOLUTE(10);
  TEST_REAL_SIMILAR(tmp_precursor.getUnchargedMass(), 13682.3053614085);
  TEST_REAL_SIMILAR(tmp_precursor.getIntensity(), 12293.4);
}
END_SECTION

START_SECTION((int getPrecursorCharge() const))
{
  int prec_cs = ms2_deconv_spec.getPrecursorCharge();
  TEST_EQUAL(prec_cs, 9);
}
END_SECTION

START_SECTION((int getPrecursorScanNumber() const))
{
  int p_scan_num = ms2_deconv_spec.getPrecursorScanNumber();
  TEST_EQUAL(p_scan_num, 0);
}
END_SECTION

START_SECTION((int getCurrentMaxAbsCharge(const int max_abs_charge) const))
{
  int tmp_cs_ms1 = test_deconv_spec.getCurrentMaxAbsCharge(5);
  int tmp_cs_ms2 = ms2_deconv_spec.getCurrentMaxAbsCharge(5);

  TEST_EQUAL(tmp_cs_ms1, 5);
  TEST_EQUAL(tmp_cs_ms2, 5);
}
END_SECTION

START_SECTION(std::string& getActivationMethod() const)
{
  Precursor::ActivationMethod act_method = ms2_deconv_spec.getActivationMethod();
  TEST_EQUAL(Precursor::ActivationMethod::ETD, act_method); // TODO: why ETD?
}
END_SECTION
////////

/// < public methods without tests > : TODOs
/// - default constructors and operators are not used (copy, move, assignment)
/// - setters (setPrecursor, etc.)
/// - updatePeakGroupQvalues
/// - nested stuff

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST
