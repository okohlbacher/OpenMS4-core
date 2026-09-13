// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest $
// --------------------------------------------------------------------------

#include <OpenMS/FORMAT/SqMassFile.h>

#include <OpenMS/FORMAT/HANDLERS/MzMLSqliteHandler.h>
#include <OpenMS/FORMAT/DATAACCESS/MSChromatogramParquetConsumer.h>
#include <OpenMS/OPENSWATHALGO/DATAACCESS/TransitionExperiment.h>

#include <algorithm>

namespace OpenMS
{

  SqMassFile::SqMassFile() = default;

  SqMassFile::~SqMassFile() = default;

  void SqMassFile::load(const std::string& filename, MapType& map) const
  {
    OpenMS::Internal::MzMLSqliteHandler sql_mass(filename, 0);
    sql_mass.setConfig(config_.write_full_meta, config_.use_lossy_numpress, config_.linear_fp_mass_acc);
    sql_mass.readExperiment(map);
  }

  void SqMassFile::store(const std::string& filename, const MapType& map) const
  {
    OpenMS::Internal::MzMLSqliteHandler sql_mass(filename, map.getSqlRunID());
    sql_mass.setConfig(config_.write_full_meta, config_.use_lossy_numpress, config_.linear_fp_mass_acc);
    sql_mass.createTables();
    sql_mass.writeExperiment(map);
  }

  void SqMassFile::transform(const std::string& filename_in, Interfaces::IMSDataConsumer* consumer, bool /* skip_full_count */, bool /* skip_first_pass */) const
  {
    OpenMS::Internal::MzMLSqliteHandler sql_mass(filename_in, 0);
    sql_mass.setConfig(config_.write_full_meta, config_.use_lossy_numpress, config_.linear_fp_mass_acc);

    // First pass through the file -> get the meta-data and hand it to the consumer
    // if (!skip_first_pass) transformFirstPass_(filename_in, consumer, skip_full_count);
    // (counted once: each count opens the file again)
    const Size nr_spectra = sql_mass.getNrSpectra();
    const Size nr_chromatograms = sql_mass.getNrChromatograms();
    consumer->setExpectedSize(nr_spectra, nr_chromatograms);
    MSExperiment experimental_settings;
    sql_mass.readExperiment(experimental_settings, true);
    consumer->setExperimentalSettings(experimental_settings);

    // Batches only while records remain: the former 'batch_idx <= count / batch_size' ran one more
    // batch with an empty index list whenever the count was a multiple of the batch size, zero
    // included. readSpectra/readChromatograms require a non-empty list, and without the
    // precondition check an empty one selected all records and then threw on the size mismatch.
    const Size batch_size = 500;
    {
      std::vector<int> indices;
      for (Size idx_start = 0; idx_start < nr_spectra; idx_start += batch_size)
      {
        const Size idx_end = std::min(idx_start + batch_size, nr_spectra);

        indices.resize(idx_end - idx_start);
        for (Size k = 0; k < indices.size(); k++)
        {
          indices[k] = static_cast<int>(idx_start + k);
        }
        std::vector<MSSpectrum> tmp_spectra;
        sql_mass.readSpectra(tmp_spectra, indices, false);
        for (Size k = 0; k < tmp_spectra.size(); k++)
        {
          consumer->consumeSpectrum(tmp_spectra[k]);
        }
      }
    }

    {
      std::vector<int> indices;
      for (Size idx_start = 0; idx_start < nr_chromatograms; idx_start += batch_size)
      {
        const Size idx_end = std::min(idx_start + batch_size, nr_chromatograms);

        indices.resize(idx_end - idx_start);
        for (Size k = 0; k < indices.size(); k++)
        {
          indices[k] = static_cast<int>(idx_start + k);
        }
        std::vector<MSChromatogram> tmp_chroms;
        sql_mass.readChromatograms(tmp_chroms, indices, false);
        for (Size k = 0; k < tmp_chroms.size(); k++)
        {
          consumer->consumeChromatogram(tmp_chroms[k]);
        }
      }
    }
  }

  void SqMassFile::convertToXICParquet(const std::string& filename_in, const std::string& xic_filename, UInt64 run_id, const std::string& source_file, const OpenSwath::LightTargetedExperiment& transition_exp) const
  {
    // source_file fallback to input filename if not provided
    std::string src = source_file.empty() ? filename_in : source_file;

    // Create an MSChromatogramParquetConsumer and stream chromatograms from the
    // sqMass file. Callers must supply a populated transition experiment;
    // chromatograms that reference missing metadata will cause the consumer to
    // throw an InvalidValue exception.
    MSChromatogramParquetConsumer parquet_consumer(xic_filename, run_id, src, transition_exp);

    // Delegate to transform which will call setExpectedSize and then stream chromatograms
    transform(filename_in, &parquet_consumer, /*skip_full_count=*/false, /*skip_first_pass=*/false);

    // Ensure writer finalizes (flush and close)
    parquet_consumer.finalize();
  }

}
