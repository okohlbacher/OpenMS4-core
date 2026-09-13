// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest $
// --------------------------------------------------------------------------

#include <OpenMS/FORMAT/HANDLERS/IndexedMzMLHandler.h>

#include <OpenMS/FORMAT/HANDLERS/IndexedMzMLDecoder.h>
#include <OpenMS/FORMAT/HANDLERS/MzMLSpectrumDecoder.h>
#include <OpenMS/CONCEPT/Exception.h>

#include <algorithm>


// #define DEBUG_READER

namespace OpenMS::Internal
{

  namespace
  {
    /**
      @brief Reads the bytes [@p startidx, @p endidx) of one record from @p stream

      Both offsets come from the file's own index and are therefore untrusted: a decreasing pair
      used to reach new char[] with a negative length (std::bad_array_new_length, which escapes
      every OpenMS exception handler) and an offset past the index allocated an arbitrary amount.
      Records always precede the \<indexList\> element, whose offset was checked against the file
      length when the footer was parsed, so @p index_offset bounds every valid record.
    */
    std::string readRecord_(std::ifstream& stream, const std::string& filename,
                            std::streampos startidx, std::streampos endidx, std::streampos index_offset)
    {
      const std::streamoff start = startidx;
      const std::streamoff end = endidx;
      if (start < 0 || end < start || end > std::streamoff(index_offset))
      {
        throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, filename,
            "Corrupt index: record byte range [" + StringUtils::toStr((long long)start) + ", "
            + StringUtils::toStr((long long)end) + ") does not lie before the index at offset "
            + StringUtils::toStr((long long)std::streamoff(index_offset)));
      }

      // a previous short read leaves failbit set, which would silently turn every later seek and read into a no-op
      stream.clear();
      stream.seekg(start, std::ios::beg);
      std::string text(static_cast<size_t>(end - start), '\0');
      stream.read(&text[0], static_cast<std::streamsize>(text.size()));
      // Keep only what was actually read: the tail used to be uninitialised memory. A short read is
      // not an error by itself (a text-mode stream on Windows consumes CRLF pairs as single characters
      // and may reach EOF first); a record that really is incomplete is rejected by the XML decoder.
      text.resize(static_cast<size_t>(std::max<std::streamsize>(stream.gcount(), 0)));
      stream.clear();
      return text;
    }
  }

  void IndexedMzMLHandler::parseFooter_()
  {
    // Opening a second file on the same handler used to append to the first file's offsets and
    // native ids, which were then resolved against the new stream. Start from an empty index, also
    // when findIndexListOffset throws or the file turns out not to be indexed.
    spectra_offsets_.clear();
    chromatograms_offsets_.clear();
    spectra_native_ids_.clear();
    chromatograms_native_ids_.clear();
    index_offset_ = (std::streampos)-1;
    spectra_before_chroms_ = true;
    parsing_success_ = false;

    //-------------------------------------------------------------
    // Find offset
    //-------------------------------------------------------------

    index_offset_ = IndexedMzMLDecoder().findIndexListOffset(filename_);
    if (index_offset_ == (std::streampos)-1)
    {
      return;
    }


    // typedef std::vector< std::pair<std::string, std::streampos> > OffsetVector;
    IndexedMzMLDecoder::OffsetVector spectra_offsets, chromatograms_offsets;
    int res = IndexedMzMLDecoder().parseOffsets(filename_, index_offset_, spectra_offsets, chromatograms_offsets);
    for (const auto& off : spectra_offsets)
    {
      spectra_native_ids_.emplace(off.first, spectra_offsets_.size());
      spectra_offsets_.push_back(off.second);
    }
    for (const auto& off : chromatograms_offsets)
    {
      chromatograms_native_ids_.emplace(off.first, chromatograms_offsets_.size());
      chromatograms_offsets_.push_back(off.second);
    }

    spectra_before_chroms_ = true;
    if (!spectra_offsets_.empty() && !chromatograms_offsets_.empty())
    {
      if (spectra_offsets_[0] < chromatograms_offsets_[0])
      {
        spectra_before_chroms_ = true;
      }
      else
      {
        spectra_before_chroms_ = false;
      }
    }

    parsing_success_ = (res == 0);
  }

  IndexedMzMLHandler::IndexedMzMLHandler(const std::string& filename) :
    index_offset_(-1),
    spectra_before_chroms_(true),
    parsing_success_(false),
    skip_xml_checks_(false)
  {
    openFile(filename);
  }

  IndexedMzMLHandler::IndexedMzMLHandler() :
    // initialised so that copying a handler which never opened a file does not read an indeterminate bool
    index_offset_(-1),
    spectra_before_chroms_(true),
    parsing_success_(false),
    skip_xml_checks_(false)
  {}

  IndexedMzMLHandler::IndexedMzMLHandler(const IndexedMzMLHandler& source) :
    filename_(source.filename_),
    spectra_offsets_(source.spectra_offsets_),
    // the native id maps are part of the index: without them every by-native-id lookup on a copy
    // (e.g. a firstprivate OnDiscMSExperiment) fails although index-based access still works
    spectra_native_ids_(source.spectra_native_ids_),
    chromatograms_offsets_(source.chromatograms_offsets_),
    chromatograms_native_ids_(source.chromatograms_native_ids_),
    index_offset_(source.index_offset_),
    spectra_before_chroms_(source.spectra_before_chroms_),
    // do not copy the filestream itself but open a new filestream using the same file
    // this is critical for parallel access to the same file!
    filestream_(source.filename_.c_str()),
    parsing_success_(source.parsing_success_),
    skip_xml_checks_(source.skip_xml_checks_)
  {
  }

  IndexedMzMLHandler::~IndexedMzMLHandler() = default;

  void IndexedMzMLHandler::openFile(const std::string& filename)
  {
    if (filestream_.is_open()) // important; otherwise opening again will fail
    {
      filestream_.close();
    }
    filename_ = filename;
    filestream_.open(filename);
    parseFooter_();
  }

  bool IndexedMzMLHandler::getParsingSuccess() const
  {
    return parsing_success_;
  }

  size_t IndexedMzMLHandler::getNrSpectra() const
  {
    return spectra_offsets_.size();
  }

  size_t IndexedMzMLHandler::getNrChromatograms() const
  {
    return chromatograms_offsets_.size();
  }

  std::string IndexedMzMLHandler::getChromatogramById_helper_(int id)
  {
    int chromToGet = id;

    if (!parsing_success_)
    {
      throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
          "Parsing was unsuccessful, cannot read file", "");
    }
    if (chromToGet < 0)
    {
      throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
          std::string( "id needs to be positive, was " + StringUtils::toStr(id) ));
    }
    if (chromToGet >= (int)getNrChromatograms())
    {
      // report the count the check was made against; the largest valid id is one below it
      throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, std::string(
            "id needs to be smaller than the number of chromatograms, was " + StringUtils::toStr(id)
            + " number of chromatograms is " + StringUtils::toStr(getNrChromatograms()) ));
    }

    std::streampos startidx = -1;
    std::streampos endidx = -1;

    if (chromToGet == int(getNrChromatograms() - 1))
    {
      startidx = chromatograms_offsets_[chromToGet];
      if (spectra_offsets_.empty() || spectra_before_chroms_)
      {
        // just take everything until the index starts
        endidx = index_offset_;
      }
      else
      {
        // just take everything until the chromatograms start
        endidx = spectra_offsets_[0];
      }
    }
    else
    {
      startidx = chromatograms_offsets_[chromToGet];
      endidx = chromatograms_offsets_[chromToGet + 1];
    }

    std::string text = readRecord_(filestream_, filename_, startidx, endidx, index_offset_);

#ifdef DEBUG_READER
    // print the full text we just read
    std::cout << text << std::endl;
#endif

    return text;
  }

  std::string IndexedMzMLHandler::getSpectrumById_helper_(int id)
  {
    int spectrumToGet = id;

    if (!parsing_success_)
    {
      throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
          "Parsing was unsuccessful, cannot read file", "");
    }
    if (spectrumToGet < 0)
    {
      throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
          std::string( "id needs to be positive, was " + StringUtils::toStr(id) ));
    }
    if (spectrumToGet >= (int)getNrSpectra())
    {
      // report the count the check was made against; the largest valid id is one below it
      throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, std::string(
            "id needs to be smaller than the number of spectra, was " + StringUtils::toStr(id)
            + " number of spectra is " + StringUtils::toStr(getNrSpectra()) ));
    }

    std::streampos startidx = -1;
    std::streampos endidx = -1;

    if (spectrumToGet == int(getNrSpectra() - 1))
    {
      startidx = spectra_offsets_[spectrumToGet];
      if (chromatograms_offsets_.empty() || !spectra_before_chroms_)
      {
        // just take everything until the index starts
        endidx = index_offset_;
      }
      else
      {
        // just take everything until the chromatograms start
        endidx = chromatograms_offsets_[0];
      }
    }
    else
    {
      startidx = spectra_offsets_[spectrumToGet];
      endidx = spectra_offsets_[spectrumToGet + 1];
    }

    std::string text = readRecord_(filestream_, filename_, startidx, endidx, index_offset_);

#ifdef DEBUG_READER
    // print the full text we just read
    std::cout << text << std::endl;
#endif

    return text;
  }

  OpenMS::Interfaces::SpectrumPtr IndexedMzMLHandler::getSpectrumById(int id)
  {
    OpenMS::Interfaces::SpectrumPtr sptr(new OpenMS::Interfaces::Spectrum);
    std::string text = IndexedMzMLHandler::getSpectrumById_helper_(id);
    MzMLSpectrumDecoder(skip_xml_checks_).domParseSpectrum(text, sptr);
    return sptr;
  }

  const OpenMS::MSSpectrum IndexedMzMLHandler::getMSSpectrumById(int id)
  {
    OpenMS::MSSpectrum s;
    getMSSpectrumById(id, s);
    return s;
  }

  void IndexedMzMLHandler::getMSSpectrumByNativeId(const std::string& id, MSSpectrum& s)
  {
    if (!spectra_native_ids_.contains(id))
    {
      throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
          std::string( "Could not find spectrum id " + std::string(id) ));
    }
    getMSSpectrumById(spectra_native_ids_[id], s);
  }

  void IndexedMzMLHandler::getMSSpectrumById(int id, MSSpectrum& s)
  {
    std::string text = IndexedMzMLHandler::getSpectrumById_helper_(id);
    MzMLSpectrumDecoder(skip_xml_checks_).domParseSpectrum(text, s);
    s.updateRanges();
  }

  OpenMS::Interfaces::ChromatogramPtr IndexedMzMLHandler::getChromatogramById(int id)
  {
    OpenMS::Interfaces::ChromatogramPtr cptr(new OpenMS::Interfaces::Chromatogram);
    std::string text = IndexedMzMLHandler::getChromatogramById_helper_(id);
    MzMLSpectrumDecoder(skip_xml_checks_).domParseChromatogram(text, cptr);
    return cptr;
  }

  const OpenMS::MSChromatogram IndexedMzMLHandler::getMSChromatogramById(int id)
  {
    OpenMS::MSChromatogram c;
    getMSChromatogramById(id, c); // already updates the ranges
    return c;
  }

  void IndexedMzMLHandler::getMSChromatogramByNativeId(const std::string& id, OpenMS::MSChromatogram& c)
  {
    auto it = chromatograms_native_ids_.find(id);
    if (it == chromatograms_native_ids_.end())
    {
      throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
          std::string("Could not find chromatogram id ") + id );
    }
    getMSChromatogramById(it->second, c);
  }

  void IndexedMzMLHandler::getMSChromatogramById(int id, MSChromatogram& c)
  {
    std::string text = IndexedMzMLHandler::getChromatogramById_helper_(id);
    MzMLSpectrumDecoder(skip_xml_checks_).domParseChromatogram(text, c);
    c.updateRanges();
  }

} //namespace OpenMS  //namespace Internal
