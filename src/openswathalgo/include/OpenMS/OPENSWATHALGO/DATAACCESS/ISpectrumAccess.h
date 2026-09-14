// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest, Witold Wolski $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/OPENSWATHALGO/OpenSwathAlgoConfig.h>

#include <OpenMS/OPENSWATHALGO/DATAACCESS/DataStructures.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace OpenMS
{
  using SpectrumSequence = std::vector<OpenSwath::SpectrumPtr>;  ///< a vector of spectrum pointers that DIA scores can operate on, allows for clever integration of only the target regions
}
namespace OpenSwath
{

  using SpectrumSequence = OpenMS::SpectrumSequence;
  /**
    @brief The interface of a mass spectrometry experiment.
  */
  class OPENSWATHALGO_DLLAPI ISpectrumAccess
  {
public:
    /// Destructor
    virtual ~ISpectrumAccess();

    /**
      @brief Light clone operator to produce a copy for concurrent read access.

      This function guarantees to produce a copy of the underlying object that
      provides thread-safe concurrent read access to the underlying data. It
      should be implemented with minimal copy-overhead to make this operation
      as fast as possible.

      To use this function, each thread should call this function to produce an
      individual copy on which it can operate.

    */
    virtual std::shared_ptr<ISpectrumAccess> lightClone() const = 0;

    /// Return a pointer to a spectrum at the given id
    virtual SpectrumPtr getSpectrumById(int id) = 0;

    /**
      @brief Return pointer to a spectrum at the given id, the spectrum will be filtered by drift time

      @throws std::invalid_argument if the spectrum has no drift time array (see filterByDrift)
    */
    SpectrumPtr getSpectrumById(int id, double drift_start, double drift_end );

    /**
      @brief Return a vector of ids of spectra that are within RT +/- deltaRT

      With @p deltaRT = 0 the first id is that of the first spectrum at or after
      @p RT, even if its RT differs from @p RT; getMultipleSpectra relies on this.
    */
    virtual std::vector<std::size_t> getSpectraByRT(double RT, double deltaRT) const = 0;
    /// Returns the number of spectra available
    virtual size_t getNrSpectra() const = 0;
    /// Returns the meta information for a spectrum
    virtual SpectrumMeta getSpectrumMetaById(int id) const = 0;

    /// Return a pointer to a chromatogram at the given id
    virtual ChromatogramPtr getChromatogramById(int id) = 0;
    /// Returns the number of chromatograms available
    virtual std::size_t getNrChromatograms() const = 0;
    /// Returns the native id of the chromatogram at the given id
    virtual std::string getChromatogramNativeID(int id) const = 0;

    /**
      @brief Fetches a spectrumSequence (multiple spectra pointers) closest to the given RT

      The sequence starts with the spectrum closest to @p RT, followed by up to
      @p nr_spectra_to_fetch / 2 (integer division) neighbours on each side, alternating
      left and right (closest, closest - 1, closest + 1, closest - 2, ...) and skipping
      neighbours outside the map. It is not sorted by RT. An odd @p nr_spectra_to_fetch
      therefore yields at most that many spectra, an even one at most
      @p nr_spectra_to_fetch + 1, and any value below 2 yields only the closest spectrum.
      The sequence is empty if no spectrum lies at or after @p RT.

      @param[in] RT target RT
      @param[in] nr_spectra_to_fetch width of the window of spectra around the target RT (see above)
    */
    SpectrumSequence getMultipleSpectra(double RT, int nr_spectra_to_fetch);

    /**
      @brief Fetches a spectrumSequence (multiple spectra pointers) closest to the given RT. Filters all spectra by specified @p drift_start and @p drift_end

      Selects the same spectra as getMultipleSpectra(double, int).

      @param[in] RT target RT
      @param[in] nr_spectra_to_fetch width of the window of spectra around the target RT
      @param[in] drift_start lower drift time bound
      @param[in] drift_end upper drift time bound
      @throws std::invalid_argument if a selected spectrum has no drift time array (see filterByDrift)
    */
    SpectrumSequence getMultipleSpectra(double RT, int nr_spectra_to_fetch, double drift_start, double drift_end);

    /**
      @brief filters a spectrum by drift time, spectrum pointer returned is a copy

      @throws std::invalid_argument if @p input lacks an m/z, intensity or drift time
              array, or if these arrays differ in length
    */
    static SpectrumPtr filterByDrift(const SpectrumPtr& input, double drift_start, double drift_end)
    {
      // NOTE: this function is very inefficient because filtering unsorted array
      if (input == nullptr)
      {
        throw std::invalid_argument("Cannot filter a missing spectrum by drift time.");
      }

      // The loop below walks the intensity and drift time arrays in step with the
      // m/z array. Spectra without ion mobility (e.g. from sqMass files) have no
      // drift time array at all, and a shorter array would be read past its end.
      OpenSwath::BinaryDataArrayPtr mz_arr = input->getMZArray();
      OpenSwath::BinaryDataArrayPtr int_arr = input->getIntensityArray();
      if (mz_arr == nullptr || int_arr == nullptr)
      {
        throw std::invalid_argument("Cannot filter by drift time: the spectrum has no m/z or intensity array.");
      }
      OpenSwath::BinaryDataArrayPtr im_arr = input->getDriftTimeArray();
      if (im_arr == nullptr)
      {
        throw std::invalid_argument("Cannot filter by drift time: the spectrum has no drift time array.");
      }
      if (int_arr->data.size() != mz_arr->data.size() || im_arr->data.size() != mz_arr->data.size())
      {
        throw std::invalid_argument("Cannot filter by drift time: the m/z, intensity and drift time arrays differ in length.");
      }

      OpenSwath::SpectrumPtr output(new OpenSwath::Spectrum);

      auto mz_it = mz_arr->data.cbegin();
      auto int_it = int_arr->data.cbegin();
      auto im_it = im_arr->data.cbegin();
      auto mz_end = mz_arr->data.cend();

      OpenSwath::BinaryDataArrayPtr mz_arr_out(new OpenSwath::BinaryDataArray);
      OpenSwath::BinaryDataArrayPtr intens_arr_out(new OpenSwath::BinaryDataArray);
      OpenSwath::BinaryDataArrayPtr im_arr_out(new OpenSwath::BinaryDataArray);
      im_arr_out->description = im_arr->description;

      while (mz_it != mz_end)
      {
        if ( (drift_start <= *im_it) && (drift_end >= *im_it) )
        {
          mz_arr_out->data.push_back( *mz_it );
          intens_arr_out->data.push_back( *int_it );
          im_arr_out->data.push_back( *im_it );
        }
        ++mz_it;
        ++int_it;
        ++im_it;
      }
      output->setMZArray(mz_arr_out);
      output->setIntensityArray(intens_arr_out);
      output->getDataArrays().push_back(im_arr_out);
      return output;
  }


   };

  typedef std::shared_ptr<ISpectrumAccess> SpectrumAccessPtr;
}

