// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest $
// --------------------------------------------------------------------------

#include <OpenMS/ANALYSIS/OPENSWATH/DATAACCESS/SpectrumAccessSqMass.h>

#include <algorithm>    // std::lower_bound, std::upper_bound, std::sort

namespace OpenMS
{
  namespace
  {
    // Maps a position in the visible view to the SQL spectrum id. Only a
    // subset view is indexed here: an unchecked position would read outside
    // the index vector, whereas a bad id in the full view is rejected by the
    // handler when no such row exists.
    int storageIndex(const std::vector<int>& sidx, int id)
    {
      if (sidx.empty())
      {
        return id;
      }
      if (id < 0 || id >= (int)sidx.size())
      {
        throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
            std::string("Spectrum index ") + id + " is outside the " + sidx.size() + " selected spectra");
      }
      return sidx[id];
    }
  }

    /// Constructor
  SpectrumAccessSqMass::SpectrumAccessSqMass(const OpenMS::Internal::MzMLSqliteHandler& handler) :
      handler_(handler)
    {}

    SpectrumAccessSqMass::SpectrumAccessSqMass(const OpenMS::Internal::MzMLSqliteHandler& handler, const std::vector<int> & indices) :
      handler_(handler),
      sidx_(indices)
    {}


    SpectrumAccessSqMass::SpectrumAccessSqMass(const SpectrumAccessSqMass& sp, const std::vector<int>& indices) :
      handler_(sp.handler_)
    {
      if (indices.empty())
      {
        sidx_ = sp.sidx_;
      }
      else if (sp.sidx_.empty())
      {
        sidx_ = indices;
      }
      else
      {
        // we only want to select a subset of the currently selected indices
        for (Size k = 0; k < indices.size(); k++)
        {
          // a negative position would convert to a huge size_t in sidx_[] below
          if (indices[k] < 0 || indices[k] >= (int)sp.sidx_.size()) throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
              std::string("Error creating SpectrumAccessSqMass with an index ") + indices[k] + " that exceeds the number of available data " + sp.sidx_.size());
          sidx_.push_back( sp.sidx_[ indices[k] ] );
        }
      }
    }

    /// Destructor
    SpectrumAccessSqMass::~SpectrumAccessSqMass() = default;

    /// Copy constructor
    SpectrumAccessSqMass::SpectrumAccessSqMass(const SpectrumAccessSqMass & rhs) :
      handler_(rhs.handler_),
      sidx_(rhs.sidx_)
    {
    }

    /// Light clone operator (actual data will not get copied)
    std::shared_ptr<OpenSwath::ISpectrumAccess> SpectrumAccessSqMass::lightClone() const
    {
      return std::shared_ptr<SpectrumAccessSqMass>(new SpectrumAccessSqMass(*this));
    }

    OpenSwath::SpectrumPtr SpectrumAccessSqMass::getSpectrumById(int id)
    {
      std::vector<int> indices;
      indices.push_back(storageIndex(sidx_, id));

      // read MSSpectra and prepare for conversion
      std::vector<MSSpectrum> tmp_spectra;
      handler_.readSpectra(tmp_spectra, indices, false);

      const MSSpectrumType& spectrum = tmp_spectra[0];
      OpenSwath::BinaryDataArrayPtr intensity_array(new OpenSwath::BinaryDataArray);
      OpenSwath::BinaryDataArrayPtr mz_array(new OpenSwath::BinaryDataArray);
      for (MSSpectrumType::const_iterator it = spectrum.begin(); it != spectrum.end(); ++it)
      {
        mz_array->data.push_back(it->getMZ());
        intensity_array->data.push_back(it->getIntensity());
      }

      OpenSwath::SpectrumPtr sptr(new OpenSwath::Spectrum);
      sptr->setMZArray(mz_array);
      sptr->setIntensityArray(intensity_array);
      return sptr;
    }

    OpenSwath::SpectrumMeta SpectrumAccessSqMass::getSpectrumMetaById(int id) const
    {
      std::vector<int> indices;
      indices.push_back(storageIndex(sidx_, id));

      // read MSSpectra and prepare for conversion
      std::vector<MSSpectrum> tmp_spectra;
      handler_.readSpectra(tmp_spectra, indices, false);

      const MSSpectrumType& spectrum = tmp_spectra[0];
      OpenSwath::SpectrumMeta m;
      // SpectrumMeta::index is the position in the list this accessor exposes
      m.index = static_cast<std::size_t>(id);
      m.id = spectrum.getNativeID();
      m.RT = spectrum.getRT();
      m.ms_level = spectrum.getMSLevel();
      return m;
    }

    void SpectrumAccessSqMass::getAllSpectra(std::vector< OpenSwath::SpectrumPtr > & spectra, std::vector< OpenSwath::SpectrumMeta > & spectra_meta) const
    {
      // read MSSpectra and prepare for conversion
      std::vector<MSSpectrum> tmp_spectra;

      if (sidx_.empty())
      {
        MSExperiment exp;
        {
          handler_.readExperiment(exp, false);
        }

        tmp_spectra = exp.getSpectra();
      }
      else
      {
        // The handler's "ID IN (...)" query returns each distinct id once, in
        // ascending id order, not in the order or multiplicity of sidx_ (a
        // repeated id even made its size check throw). Read every distinct id
        // once and rebuild the configured view, so that it matches what
        // getSpectrumById() returns for each position.
        std::vector<int> unique_ids(sidx_);
        std::sort(unique_ids.begin(), unique_ids.end());
        unique_ids.erase(std::unique(unique_ids.begin(), unique_ids.end()), unique_ids.end());

        handler_.readSpectra(tmp_spectra, unique_ids, false);

        if (unique_ids != sidx_)
        {
          std::vector<MSSpectrum> unique_spectra;
          unique_spectra.swap(tmp_spectra);
          tmp_spectra.reserve(sidx_.size());
          for (Size k = 0; k < sidx_.size(); k++)
          {
            Size pos = std::lower_bound(unique_ids.begin(), unique_ids.end(), sidx_[k]) - unique_ids.begin();
            tmp_spectra.push_back(unique_spectra[pos]);
          }
        }
      }
      spectra.reserve(tmp_spectra.size());
      spectra_meta.reserve(tmp_spectra.size());
    
      for (Size k = 0; k < tmp_spectra.size(); k++)
      {
        const MSSpectrumType& spectrum = tmp_spectra[k];
        OpenSwath::BinaryDataArrayPtr intensity_array(new OpenSwath::BinaryDataArray);
        OpenSwath::BinaryDataArrayPtr mz_array(new OpenSwath::BinaryDataArray);
        for (MSSpectrumType::const_iterator it = spectrum.begin(); it != spectrum.end(); ++it)
        {
          mz_array->data.push_back(it->getMZ());
          intensity_array->data.push_back(it->getIntensity());
        }

        OpenSwath::SpectrumMeta m;
        m.index = k;
        m.id = spectrum.getNativeID();
        m.RT = spectrum.getRT();
        m.ms_level = spectrum.getMSLevel();
        spectra_meta.push_back(m);

        OpenSwath::SpectrumPtr sptr(new OpenSwath::Spectrum);
        sptr->setMZArray(mz_array);
        sptr->setIntensityArray(intensity_array);
        spectra.push_back(sptr);
      }
    }

    std::vector<std::size_t> SpectrumAccessSqMass::getSpectraByRT(double RT, double deltaRT) const
    {
      OPENMS_PRECONDITION(deltaRT >= 0, "Delta RT needs to be a positive number");
      std::vector<std::size_t> res = handler_.getSpectraIndicesbyRT(RT, deltaRT, sidx_);

      if (sidx_.empty())
      {
        return res;
      }
      else
      {
        // we need to map the resulting indices back to the external indices
        std::vector<std::size_t> res_mapped;
        for (Size k = 0; k < res.size(); k++)
        {
          for (Size s_it = 0; s_it < sidx_.size(); s_it++)
          {
            if (res[k] == (size_t)sidx_[s_it]) {res_mapped.push_back(s_it);}
          }
        }
        return res_mapped;
      }
    }

    size_t SpectrumAccessSqMass::getNrSpectra() const
    {
      size_t res;
      if (sidx_.empty())
      {
        res = handler_.getNrSpectra();
      }
      else
      {
        res = sidx_.size();
      }
      return res;
    }

    OpenSwath::ChromatogramPtr SpectrumAccessSqMass::getChromatogramById(int /* id */)
    {
      throw Exception::NotImplemented(__FILE__,__LINE__,OPENMS_PRETTY_FUNCTION);
    }

    size_t SpectrumAccessSqMass::getNrChromatograms() const
    {
      size_t res;
      // TODO: currently chrom indices are not supported
      res = handler_.getNrChromatograms();
      return res;
    }

    std::string SpectrumAccessSqMass::getChromatogramNativeID(int /* id */) const
    {
      throw Exception::NotImplemented(__FILE__,__LINE__,OPENMS_PRETTY_FUNCTION);
    }

} //end namespace OpenMS
