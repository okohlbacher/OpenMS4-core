// Copyright (c) 2002-present, The OpenMS Team -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Kyowon Jeong $
// $Authors: Kyowon Jeong $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/ANALYSIS/TOPDOWN/DeconvolvedSpectrum.h>
#include <OpenMS/ANALYSIS/TOPDOWN/FLASHHelperClasses.h>
#include <OpenMS/KERNEL/Peak1D.h>
#include <OpenMS/METADATA/Precursor.h>


namespace OpenMS
{
  class PeakGroup;

  /**
@brief scoring functions for PeakGroup.
   For now, only Qscore has been implemented. For Qscore,
   the weight vector has been determined by logistic regression.
   In the future, other technique such as deep learning would be used.
@ingroup Topdown
*/

  class OPENMS_DLLAPI PeakGroupScoring
  {
  public:
    typedef FLASHHelperClasses::LogMzPeak LogMzPeak;

    /// Minimum isotopologue count accepted by isotope scoring.
    static constexpr int MIN_ISOTOPE_COUNT = 2;

    /**
      @brief Calculate the cosine score for a measured intensity window and an isotope distribution.
      @param[in] a Measured isotope intensities.
      @param[in] a_start First included measured index, clamped to zero.
      @param[in] a_end Exclusive final measured index, clamped to the vector size.
      @param[in] b Reference isotope distribution.
      @param[in] offset Index shift from measured to reference intensities.
      @param[in] min_iso_len Minimum accepted number of measured indices.
      @return Cosine score, or zero for an insufficient or zero-intensity window.
    */
    static float getCosine(const std::vector<float>& a, int a_start, int a_end,
                           const IsotopeDistribution& b, int offset, int min_iso_len);

    /**
      @brief Score an isotope pattern and find its best monoisotopic offset.
      @param[in] mono_mass Initial monoisotopic mass.
      @param[in] per_isotope_intensities Measured isotope intensities summed across charges.
      @param[out] offset Selected offset relative to the initial isotope shift.
      @param[in] avg Reference averagine distributions.
      @param[in] iso_int_shift Initial isotope shift.
      @param[in] window_width Offset search window; negative selects the automatic window.
      @param[in] excluded_masses Mass hypotheses to exclude.
      @return Best cosine score using the existing deconvolution scoring convention.
    */
    static float getIsotopeCosineAndIsoOffset(double mono_mass,
      const std::vector<float>& per_isotope_intensities, int& offset,
      const FLASHHelperClasses::PrecalculatedAveragine& avg, int iso_int_shift,
      int window_width, const std::vector<double>& excluded_masses);

    /// get QScore for a peak group of specific abs_charge
    static double getQscore(const PeakGroup* pg);

    /// Write Csv file for Qscore training.
    static void writeAttCsvForQscoreTraining(const DeconvolvedSpectrum& deconvolved_spectrum, std::fstream& f);

    /// Write Csv file for Qscore training header.
    static void writeAttCsvForQscoreTrainingHeader(std::fstream& f);

    /// get Deep learning based peak group score. Not implemented yet.
    static double getDLscore(PeakGroup* pg, const MSSpectrum& spec, const FLASHHelperClasses::PrecalculatedAveragine& avg, double tol);

  private:
    /// convert a peak group to a feature vector for setQscore calculation
    static std::vector<double> toFeatureVector_(const PeakGroup* pg);
    /// the weights for Qscore calculation
    static std::vector<double> weight_;

    /// charge and isotope counts for DL scoring.
    static const int charge_count_for_DL_scoring_ = 11;
    static const int iso_count_for_DL_scoring_ = 13;
  };
} // namespace OpenMS