// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/INTERFACES/IMSDataConsumer.h>

#include <OpenMS/KERNEL/MSExperiment.h>

#include <memory>

namespace OpenMS
{

    namespace Internal
    {
      class MzMLSqliteHandler;
    }

    /**
      @brief A data consumer that inserts MS data into a SQLite database

      Consumes spectra and chromatograms and inserts them into an file-based
      SQL database using SQLite. As SQLite is highly inefficient when inserting
      one spectrum/chromatogram at a time, the consumer collects the data in an
      internal buffer and then flushes them all together to disk.

      It uses MzMLSqliteHandler internally to write batches of data to disk.

    */
    class OPENMS_DLLAPI MSDataSqlConsumer :
      public Interfaces::IMSDataConsumer
    {
      typedef MSExperiment MapType;
      typedef MapType::SpectrumType SpectrumType;
      typedef MapType::ChromatogramType ChromatogramType;

    public:

      /**
        @brief Constructor

        Opens the SQLite file and writes the tables.

        @param[in] sql_filename The filename of the SQLite database (an existing file is replaced)
        @param[in] run_id Unique identifier which links the sqMass and OSW file
        @param[in] buffer_size How large the internal buffer size should be (defaults to 500 spectra / chromatograms); 0 writes every record immediately
        @param[in] full_meta Whether to write the full meta-data in the SQLite header
        @param[in] lossy_compression Whether to use lossy compression (numpress)
        @param[in] linear_mass_acc Desired mass accuracy for RT or m/z space (absolute value)

        @throws Exception::IllegalArgument if @p buffer_size is negative (checked before the file is touched)
      */
      MSDataSqlConsumer(const std::string& sql_filename, UInt64 run_id, int buffer_size = 500, bool full_meta = true, bool lossy_compression=false, double linear_mass_acc=1e-4);

      /// The consumer owns its database handler, so it cannot be copied
      MSDataSqlConsumer(const MSDataSqlConsumer&) = delete;
      /// The consumer owns its database handler, so it cannot be copied
      MSDataSqlConsumer& operator=(const MSDataSqlConsumer&) = delete;

      /**
        @brief Destructor

        Calls finalize(). A destructor cannot report an error, so a failure
        to write is only logged here; call finalize() first to handle it.
      */
      ~MSDataSqlConsumer() override;

      /**
        @brief Flushes the data for good.

        After calling this function, no more data is held in the buffer but the
        class is still able to receive new data.
      */
      void flush();

      /**
        @brief Flush all buffered data and write the run-level information

        Writes the RUN entry for the current run id (with the full meta-data
        snapshot if requested), unless addRun() has already registered a run.
        Calling it again only flushes data consumed since.

        @throws Exception::BaseException if writing to the database fails
      */
      void finalize();

      /**
        @brief Add/insert a RUN entry into the sqMass file (ID and filename)

        Buffered records are flushed first, so they keep the run id under
        which they were consumed. The RUN entry is written immediately: with
        @c full_meta, its meta-data snapshot only holds @p filename, and the
        meta-data of records consumed afterwards is not stored (reading such a
        file falls back to the columns of the SQL tables).
      */
      void addRun(const std::string& filename, const UInt64 run_id);

      /**
        @brief Change the current run id used for subsequent chromatogram/spectrum writes

        Buffered records are flushed first, so they keep the run id under
        which they were consumed.
      */
      void setRunId(const UInt64 run_id);

      /**
        @brief Write a spectrum to the output file
      */
      void consumeSpectrum(SpectrumType & s) override;

      /**
        @brief Write a chromatogram to the output file
      */
      void consumeChromatogram(ChromatogramType & c) override;

      void setExpectedSize(Size /* expectedSpectra */, Size /* expectedChromatograms */) override;

      /**
        @brief Set the experimental settings stored with the full meta-data

        Only used if the consumer was constructed with @c full_meta.
      */
      void setExperimentalSettings(const ExperimentalSettings& exp) override;

    protected:

      std::string filename_;
      std::unique_ptr<OpenMS::Internal::MzMLSqliteHandler> handler_;

      size_t flush_after_;
      bool full_meta_;
      std::vector<SpectrumType> spectra_;
      std::vector<ChromatogramType> chromatograms_;

      MSExperiment peak_meta_;
      bool wrote_any_run_ = false;
    };

} //end namespace OpenMS


