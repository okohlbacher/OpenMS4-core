// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest $
// --------------------------------------------------------------------------

#include <OpenMS/FORMAT/DATAACCESS/MSDataSqlConsumer.h>

#include <OpenMS/CONCEPT/Exception.h>
#include <OpenMS/CONCEPT/LogStream.h>
#include <OpenMS/FORMAT/HANDLERS/MzMLSqliteHandler.h>

namespace OpenMS
{

  MSDataSqlConsumer::MSDataSqlConsumer(const std::string& filename, UInt64 run_id, int flush_after, bool full_meta, bool lossy_compression, double linear_mass_acc) :
        filename_(filename),
        handler_(new OpenMS::Internal::MzMLSqliteHandler(filename, run_id) ),
        flush_after_(flush_after),
        full_meta_(full_meta)
  {
    // A negative size would wrap to a huge size_t buffer; reject it before
    // createTables() replaces an existing file.
    if (flush_after < 0)
    {
      throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
          std::string("Buffer size must not be negative, got ") + flush_after);
    }

    spectra_.reserve(flush_after_);
    chromatograms_.reserve(flush_after_);

    handler_->setConfig(full_meta, lossy_compression, linear_mass_acc, flush_after);
    handler_->createTables();
  }

  MSDataSqlConsumer::~MSDataSqlConsumer()
  {
    // An exception escaping a destructor terminates the program, so a failed
    // final write can only be reported here; finalize() lets callers handle it.
    try
    {
      finalize();
    }
    catch (const std::exception& e)
    {
      OPENMS_LOG_ERROR << "Failed to write sqMass file '" << filename_ << "': " << e.what() << std::endl;
    }
    catch (...)
    {
      OPENMS_LOG_ERROR << "Failed to write sqMass file '" << filename_ << "': unknown exception." << std::endl;
    }
  }

  void MSDataSqlConsumer::finalize()
  {
    flush();

    // Write run level information into the file (e.g. run id, run name and mzML structure)
    // Only write run-level information if a run hasn't already been written
    if (!wrote_any_run_)
    {
      peak_meta_.setLoadedFilePath(filename_);
      handler_->writeRunLevelInformation(peak_meta_, full_meta_);
      wrote_any_run_ = true;
    }
  }

  void MSDataSqlConsumer::addRun(const std::string& filename, const UInt64 run_id)
  {
    // the buffers do not record their run, so write them under the id they were consumed with
    flush();

    // set handler's run id and write run level information
    handler_->setRunId(run_id);
    MSExperiment meta;
    meta.setLoadedFilePath(filename);
    handler_->writeRunLevelInformation(meta, full_meta_);
    wrote_any_run_ = true;
  }

  void MSDataSqlConsumer::setRunId(const UInt64 run_id)
  {
    // the buffers do not record their run, so write them under the id they were consumed with
    flush();
    handler_->setRunId(run_id);
  }

  void MSDataSqlConsumer::flush()
  {
    if (!spectra_.empty() ) 
    {
      handler_->writeSpectra(spectra_);
      spectra_.clear();
      spectra_.reserve(flush_after_);
    }

    if (!chromatograms_.empty() ) 
    {
      handler_->writeChromatograms(chromatograms_);
      chromatograms_.clear();
      chromatograms_.reserve(flush_after_);
    }
  }

  void MSDataSqlConsumer::consumeSpectrum(SpectrumType & s)
  {
    spectra_.push_back(s);
    s.clear(false);
    if (full_meta_)
    {
      peak_meta_.addSpectrum(s);
    }
    if (spectra_.size() >= flush_after_)
    {
      flush();
    }
  }

  void MSDataSqlConsumer::consumeChromatogram(ChromatogramType & c)
  {
    chromatograms_.push_back(c);
    c.clear(false);
    if (full_meta_)
    {
      peak_meta_.addChromatogram(c);
    }
    if (chromatograms_.size() >= flush_after_)
    {
      flush();
    }
  }

  void MSDataSqlConsumer::setExpectedSize(Size /* expectedSpectra */, Size /* expectedChromatograms */) {;}

  void MSDataSqlConsumer::setExperimentalSettings(const ExperimentalSettings& exp)
  {
    // the full meta-data snapshot promises the complete input structure, which
    // includes the run's settings and not only the spectrum/chromatogram headers
    if (full_meta_)
    {
      static_cast<ExperimentalSettings&>(peak_meta_) = exp;
    }
  }

} // namespace OpenMS

