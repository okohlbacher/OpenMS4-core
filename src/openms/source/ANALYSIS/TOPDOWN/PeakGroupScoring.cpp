// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Kyowon Jeong$
// $Authors: Kyowon Jeong$
// --------------------------------------------------------------------------

#include <OpenMS/ANALYSIS/TOPDOWN/FLASHHelperClasses.h>
#include <fstream>
#include <OpenMS/ANALYSIS/TOPDOWN/PeakGroup.h>
#include <OpenMS/ANALYSIS/TOPDOWN/PeakGroupScoring.h>


namespace OpenMS
  {
    std::vector<double> PeakGroupScoring::weight_ { -21.0476, 1.5045, -0.1303, 0.183, 0.1834, 17.804};
    // Att0                21.0476
    // Att1                -1.5045
    // Att2                 0.1303
    // Att3                 -0.183
    // Att4                -0.1834
    // Intercept           -17.804

    /// calculate PeakGroupScoring using PeakGroup attributes
    double PeakGroupScoring::getQscore(const PeakGroup* pg)
    {
      if (pg->empty())
      { // all zero
        return .0;
      }

      double score = weight_.back() + .5;
      auto fv = toFeatureVector_(pg);

      for (Size i = 0; i < weight_.size() - 1; i++)
      {
        score += fv[i] * weight_[i];
      }
      double qscore = 1.0 / (1.0 + exp(score));

      return qscore;
    }

    /// convert PeakGroup into feature (attribute) vector
    std::vector<double> PeakGroupScoring::toFeatureVector_(const PeakGroup* pg)
    {
      std::vector<double> fvector(5, .0); // length of weights vector - 1, excluding the intercept weight.
      if (pg->empty())
        return fvector;
      int index = 0;
      fvector[index++] = pg->getIsotopeCosine(); // (log2(a + d));

      fvector[index++] = pg->getIsotopeCosine() - pg->getChargeIsotopeCosine(pg->getRepAbsCharge()); // (log2(d + a / (d + a)));

      fvector[index++] = log2(1 + pg->getChargeSNR(pg->getRepAbsCharge())); //(log2(d + a / (d + a)));

      fvector[index++] = log2(1 + pg->getChargeSNR(pg->getRepAbsCharge())) - log2(1 + pg->getSNR()); //(log2(a + d));

      fvector[index++] = pg->getAvgPPMError();

      return fvector;
    }

    /// to write down training csv file header.
    void PeakGroupScoring::writeAttCsvForQscoreTrainingHeader(std::fstream& f)
    {
      PeakGroup pg;
      Size att_count = toFeatureVector_(&pg).size();
      for (Size i = 0; i < att_count; i++)
        f << "Att" << i << ",";
      f << "Class\n";
    }

    /// to write down training csv file rows
    void PeakGroupScoring::writeAttCsvForQscoreTraining(const DeconvolvedSpectrum& deconvolved_spectrum, std::fstream& f)
    {
      DeconvolvedSpectrum dspec;
      dspec.reserve(deconvolved_spectrum.size());
      for (auto& pg : deconvolved_spectrum)
      {
        dspec.push_back(pg);
      }

      if (dspec.empty())
        return;

      for (auto& pg : dspec)
      {
        bool target = pg.getTargetDecoyType() == PeakGroup::TargetDecoyType::target;
        auto fv = toFeatureVector_(&pg);

        for (auto& item : fv)
        {
          f << item << ",";
        }
        f << (target ? "T" : "F") << "\n";
      }
    }

    double PeakGroupScoring::getDLscore(PeakGroup* pg, const MSSpectrum& spec, const FLASHHelperClasses::PrecalculatedAveragine& avg, double tol)
    {
      const auto& [sig, noise]
                  = pg->getDLVector(spec, charge_count_for_DL_scoring_, iso_count_for_DL_scoring_, avg, tol);

      /// calculate score with sig and  noise


      return 0;
    }

    float PeakGroupScoring::getIsotopeCosineAndIsoOffset(double mono_mass,
                                                            const std::vector<float>& per_isotope_intensities,
                                                            int& offset,
                                                            const FLASHHelperClasses::PrecalculatedAveragine& avg,
                                                            const int iso_int_shift,
                                                            const int window_width,
                                                            const std::vector<double>& excluded_masses)
  {
    offset = 0;
    if ((int)per_isotope_intensities.size() < MIN_ISOTOPE_COUNT + iso_int_shift) { return .0; }
    auto iso = avg.get(mono_mass);

    int right = (int)avg.getApexIndex(mono_mass) / 4 + 1;
    int left = right;

    right += iso_int_shift;
    left -= iso_int_shift;
    float max_cos = -1000;
    int max_isotope_index = (int)per_isotope_intensities.size(); // exclusive
    int min_isotope_index = -1;                                  // inclusive

    for (int i = 0; i < max_isotope_index; i++)
    {
      if (per_isotope_intensities[i] <= 0) { continue; }

      if (min_isotope_index < 0) { min_isotope_index = i; }
    }
    if (max_isotope_index - min_isotope_index < MIN_ISOTOPE_COUNT) { return .0; }

    std::vector<std::pair<int, float>> offset_cos;
    offset_cos.reserve(right + left + 1);

    for (int tmp_offset = -left; tmp_offset <= right; tmp_offset++)
    {
      if (window_width >= 0 && abs(tmp_offset - iso_int_shift) > window_width)
        continue;
      if (!excluded_masses.empty())
      {
        bool exclude = false;
        for (auto em : excluded_masses)
        {
          if ( std::abs(mono_mass + (tmp_offset - iso_int_shift)* Constants::ISOTOPE_MASSDIFF_55K_U - em) < mono_mass * 1e-5) // tmp
          {
            exclude = true;
            break;
          }
        }
        if (exclude) continue;
      }
      float tmp_cos = getCosine(per_isotope_intensities, min_isotope_index, max_isotope_index, iso, tmp_offset, MIN_ISOTOPE_COUNT);
      offset_cos.emplace_back(tmp_offset, tmp_cos);
    }
    if (offset_cos.empty()) return max_cos;

    std::sort(offset_cos.begin(), offset_cos.end(),
              [](const std::pair<int, float>& p1, const std::pair<int, float>& p2) { return p1.second > p2.second; });

    for (const auto& [o, c] : offset_cos)
    {
      if (o > right || o < -left) continue;
      if (window_width >= 0 && abs(o - iso_int_shift) > window_width) //
        continue;

      offset = o;
      max_cos = c;
      break;
    }

    max_cos = std::max(max_cos, .0f);
    offset -= iso_int_shift;

    return max_cos;
  }

  float PeakGroupScoring::getCosine(const std::vector<float>& a, int a_start, int a_end, const IsotopeDistribution& b, int offset, int min_iso_len)
  {
    float n = .0, a_norm = .0, b_norm = 1.0f;
    a_start = std::max(0, a_start);
    a_end = std::min((int)a.size(), a_end);

    if (a_end - a_start < min_iso_len) { return 0; }

    float max_intensity = 0;

    for (int j = a_start; j < a_end; j++)
    {
      int i = j - offset;
      a_norm += a[j] * a[j];

      if (max_intensity < a[j]) { max_intensity = a[j]; }

      if (i >= (int)b.size() || i < 0 || b[i].getIntensity() <= 0) { continue; }
      else { n += a[j] * b[i].getIntensity(); }
    }

    if (a_norm <= 0) { return 0; }

    return n / sqrt(a_norm * b_norm);
  }

} // namespace OpenMS