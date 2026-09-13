// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Aditya Sarna $
// --------------------------------------------------------------------------

#include <OpenMS/FORMAT/HANDLERS/ImzMLHandlerHelper.h>
#include <OpenMS/CONCEPT/Exception.h>
#include <OpenMS/CONCEPT/Types.h>
#include <OpenMS/SYSTEM/File.h>

#include <cstdint>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <cstring>
#include <limits>

#ifndef OPENMS_IS_BIG_ENDIAN
#if defined(OPENMS_BIG_ENDIAN)
#define OPENMS_IS_BIG_ENDIAN 1
#else
#define OPENMS_IS_BIG_ENDIAN 0
#endif
#endif

namespace OpenMS
{

namespace
{
  void throwReadError_(const std::string& ibd_path, const std::string& detail)
  {
    throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, ibd_path, detail);
  }

  /// Guard against malformed imzML metadata requesting huge vector allocations.
  static constexpr uint64_t MAX_IBD_ARRAY_ELEMENTS = 100'000'000ULL;

  void validateCount_(const uint64_t count, const std::string& ibd_path, const char* context)
  {
    if (count > MAX_IBD_ARRAY_ELEMENTS)
    {
      throwReadError_(ibd_path,
                      std::string(context) + ": element count " + OpenMS::StringConversions::toString(count) + " exceeds limit of "
                      + OpenMS::StringConversions::toString(MAX_IBD_ARRAY_ELEMENTS));
    }
    if (count > static_cast<uint64_t>(std::numeric_limits<Size>::max()))
    {
      throwReadError_(ibd_path, std::string(context) + ": element count exceeds platform limit");
    }
  }

  /// Stored width in bytes of one array element; 0 for an unsupported/unknown type.
  uint64_t elementWidth_(const ImzMLSpectrumIndex::DataType dt)
  {
    switch (dt)
    {
      case ImzMLSpectrumIndex::DataType::FLOAT32:
      case ImzMLSpectrumIndex::DataType::INT32:
        return 4;
      case ImzMLSpectrumIndex::DataType::FLOAT64:
      case ImzMLSpectrumIndex::DataType::INT64:
        return 8;
      default:
        return 0;
    }
  }

  /// Reject a declared array range that the .ibd cannot answer, *before* the output vector
  /// is sized from it: offset and count come from the .imzML while the bytes live in the
  /// companion .ibd, so the file's own length is the only fact that makes the request
  /// answerable. Without this, a malformed IMS:1000103 commits the full allocation (up to
  /// MAX_IBD_ARRAY_ELEMENTS elements, plus the staging vector of the widening readers) and
  /// learns of the truncation only when fread comes up short. The length comes from fstat() on
  /// the open handle: it leaves the handle's position and stdio buffer untouched, and unlike a
  /// stat of the path it costs no filesystem lookup on this per-array hot path.
  void validateRange_(FILE* ibd,
                      const uint64_t offset,
                      const uint64_t count,
                      const ImzMLSpectrumIndex::DataType dt,
                      const std::string& ibd_path,
                      const char* context)
  {
    const uint64_t width = elementWidth_(dt);
    if (width == 0)
    {
      throwReadError_(ibd_path, std::string("unsupported ") + context + " data type in .ibd");
    }
    // count is already bounded by MAX_IBD_ARRAY_ELEMENTS and width by 8, so the product
    // cannot overflow; only the offset (unbounded in the XML) can push the end past uint64.
    const uint64_t bytes = count * width;
    if (offset > std::numeric_limits<uint64_t>::max() - bytes)
    {
      throwReadError_(ibd_path, std::string(context) + ": byte offset "
                      + OpenMS::StringConversions::toString(offset) + " plus "
                      + OpenMS::StringConversions::toString(bytes) + " bytes overflows");
    }
#ifdef _WIN32
    struct _stat64 st;
    const bool have_length = _fstat64(_fileno(ibd), &st) == 0;
#else
    struct stat st;
    const bool have_length = fstat(fileno(ibd), &st) == 0;
#endif
    if (!have_length || st.st_size < 0)
    { // cannot tell the length: the short-read check in the caller remains the guard
      return;
    }
    const uint64_t length = static_cast<uint64_t>(st.st_size);
    if (offset + bytes > length)
    {
      throwReadError_(ibd_path, std::string(context) + ": declared range [offset "
                      + OpenMS::StringConversions::toString(offset) + ", "
                      + OpenMS::StringConversions::toString(bytes) + " bytes] extends past the .ibd length "
                      + OpenMS::StringConversions::toString(length));
    }
  }

  void seekIbd_(FILE* ibd, const uint64_t offset, const std::string& ibd_path, const char* context)
  {
#ifdef WIN32
    if (offset > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
    {
      throwReadError_(ibd_path,
                      std::string(context) + ": byte offset " + OpenMS::StringConversions::toString(offset) + " exceeds platform seek limit");
    }
    if (_fseeki64(ibd, static_cast<int64_t>(offset), SEEK_SET) != 0)
#else
    if (offset > static_cast<uint64_t>(std::numeric_limits<off_t>::max()))
    {
      throwReadError_(ibd_path,
                      std::string(context) + ": byte offset " + OpenMS::StringConversions::toString(offset) + " exceeds platform seek limit");
    }
    if (fseeko(ibd, static_cast<off_t>(offset), SEEK_SET) != 0)
#endif
    {
      throwReadError_(ibd_path, std::string(context) + ": failed to seek in .ibd");
    }
  }

#if OPENMS_IS_BIG_ENDIAN
  inline uint32_t swapU32_(const uint32_t v)
  {
    return ((v & 0x000000ffU) << 24) | ((v & 0x0000ff00U) << 8) | ((v & 0x00ff0000U) >> 8)
           | ((v & 0xff000000U) >> 24);
  }

  inline uint64_t swapU64_(const uint64_t v)
  {
    return ((v >> 56) & 0x00000000000000FFULL) | ((v >> 40) & 0x000000000000FF00ULL)
           | ((v >> 24) & 0x0000000000FF0000ULL) | ((v >> 8) & 0x00000000FF000000ULL)
           | ((v << 8) & 0x000000FF00000000ULL) | ((v << 24) & 0x0000FF0000000000ULL)
           | ((v << 40) & 0x00FF000000000000ULL) | ((v << 56) & 0xFF00000000000000ULL);
  }

#endif

  /// imzML .ibd arrays are little-endian (imzML 1.1.0); byte-swap on big-endian hosts.
  template<typename T>
  void decodeLittleEndian_([[maybe_unused]] T* data, [[maybe_unused]] const Size count)
  {
#if OPENMS_IS_BIG_ENDIAN
    if constexpr (sizeof(T) == 4)
    {
      auto* raw = reinterpret_cast<uint32_t*>(data);
      for (Size i = 0; i < count; ++i)
      {
        raw[i] = swapU32_(raw[i]);
      }
    }
    else if constexpr (sizeof(T) == 8)
    {
      auto* raw = reinterpret_cast<uint64_t*>(data);
      for (Size i = 0; i < count; ++i)
      {
        raw[i] = swapU64_(raw[i]);
      }
    }
#endif
  }
} // namespace

void ImzMLBinaryIO::readMzArray(FILE* ibd,
                                const uint64_t offset,
                                const uint64_t count,
                                const ImzMLSpectrumIndex::DataType dt,
                                std::vector<double>& out,
                                const std::string& ibd_path)
{
  out.clear();
  if (!ibd || count == 0)
  {
    return;
  }

  validateCount_(count, ibd_path, "m/z array");
  validateRange_(ibd, offset, count, dt, ibd_path, "m/z array");

  seekIbd_(ibd, offset, ibd_path, "m/z array");

  out.resize(count);

  switch (dt)
  {
    case ImzMLSpectrumIndex::DataType::FLOAT32:
    {
      std::vector<float> tmp(count);
      if (fread(tmp.data(), 4, count, ibd) != count)
      {
        throwReadError_(ibd_path, "failed to read float32 m/z array from .ibd");
      }
      decodeLittleEndian_(tmp.data(), count);
      for (Size i = 0; i < count; ++i)
      {
        out[i] = tmp[i];
      }
      break;
    }
    case ImzMLSpectrumIndex::DataType::FLOAT64:
      if (fread(out.data(), 8, count, ibd) != count)
      {
        throwReadError_(ibd_path, "failed to read float64 m/z array from .ibd");
      }
      decodeLittleEndian_(out.data(), count);
      break;
    case ImzMLSpectrumIndex::DataType::INT32:
    {
      std::vector<int32_t> tmp(count);
      if (fread(tmp.data(), 4, count, ibd) != count)
      {
        throwReadError_(ibd_path, "failed to read int32 m/z array from .ibd");
      }
      decodeLittleEndian_(tmp.data(), count);
      for (Size i = 0; i < count; ++i)
      {
        out[i] = tmp[i];
      }
      break;
    }
    case ImzMLSpectrumIndex::DataType::INT64:
    {
      std::vector<int64_t> tmp(count);
      if (fread(tmp.data(), 8, count, ibd) != count)
      {
        throwReadError_(ibd_path, "failed to read int64 m/z array from .ibd");
      }
      decodeLittleEndian_(tmp.data(), count);
      for (Size i = 0; i < count; ++i)
      {
        out[i] = static_cast<double>(tmp[i]);
      }
      break;
    }
    default:
      throwReadError_(ibd_path, "unsupported m/z array data type in .ibd");
  }
}

namespace
{
  /// Shared float-typed .ibd array reader. @p context names the array in error messages.
  void readFloatVector_(FILE* ibd,
                        const uint64_t offset,
                        const uint64_t count,
                        const ImzMLSpectrumIndex::DataType dt,
                        std::vector<float>& out,
                        const std::string& ibd_path,
                        const std::string& context)
  {
    out.clear();
    if (!ibd || count == 0)
    {
      return;
    }

    validateCount_(count, ibd_path, context.c_str());
    validateRange_(ibd, offset, count, dt, ibd_path, context.c_str());

    seekIbd_(ibd, offset, ibd_path, context.c_str());

    out.resize(count);

    switch (dt)
    {
      case ImzMLSpectrumIndex::DataType::FLOAT32:
        if (fread(out.data(), 4, count, ibd) != count)
        {
          throwReadError_(ibd_path, "failed to read float32 " + context + " from .ibd");
        }
        decodeLittleEndian_(out.data(), count);
        break;
      case ImzMLSpectrumIndex::DataType::FLOAT64:
      {
        std::vector<double> tmp(count);
        if (fread(tmp.data(), 8, count, ibd) != count)
        {
          throwReadError_(ibd_path, "failed to read float64 " + context + " from .ibd");
        }
        decodeLittleEndian_(tmp.data(), count);
        for (Size i = 0; i < count; ++i)
        {
          out[i] = static_cast<float>(tmp[i]);
        }
        break;
      }
      case ImzMLSpectrumIndex::DataType::INT32:
      {
        std::vector<int32_t> tmp(count);
        if (fread(tmp.data(), 4, count, ibd) != count)
        {
          throwReadError_(ibd_path, "failed to read int32 " + context + " from .ibd");
        }
        decodeLittleEndian_(tmp.data(), count);
        for (Size i = 0; i < count; ++i)
        {
          out[i] = static_cast<float>(tmp[i]);
        }
        break;
      }
      case ImzMLSpectrumIndex::DataType::INT64:
      {
        std::vector<int64_t> tmp(count);
        if (fread(tmp.data(), 8, count, ibd) != count)
        {
          throwReadError_(ibd_path, "failed to read int64 " + context + " from .ibd");
        }
        decodeLittleEndian_(tmp.data(), count);
        for (Size i = 0; i < count; ++i)
        {
          out[i] = static_cast<float>(tmp[i]);
        }
        break;
      }
      default:
        throwReadError_(ibd_path, "unsupported " + context + " data type in .ibd");
    }
  }
} // namespace

void ImzMLBinaryIO::readIntArray(FILE* ibd,
                                 const uint64_t offset,
                                 const uint64_t count,
                                 const ImzMLSpectrumIndex::DataType dt,
                                 std::vector<float>& out,
                                 const std::string& ibd_path)
{
  readFloatVector_(ibd, offset, count, dt, out, ibd_path, "intensity array");
}

void ImzMLBinaryIO::readAuxArray(FILE* ibd,
                                 const uint64_t offset,
                                 const uint64_t count,
                                 const ImzMLSpectrumIndex::DataType dt,
                                 std::vector<float>& out,
                                 const std::string& ibd_path,
                                 const std::string& array_name)
{
  readFloatVector_(ibd, offset, count, dt, out, ibd_path,
                   array_name.empty() ? std::string("auxiliary array") : array_name);
}

void ImzMLBinaryIO::writeFloat32Array(FILE* ibd,
                                      const float* data,
                                      const uint64_t count,
                                      const std::string& ibd_path)
{
  if (!ibd)
  {
    throwReadError_(ibd_path, "invalid .ibd handle (null FILE*) in float32 writer");
  }
  if (count == 0)
  {
    return;
  }
  validateCount_(count, ibd_path, "float32 array write");
#if OPENMS_IS_BIG_ENDIAN
  std::vector<float> le_data(static_cast<size_t>(count));
  std::memcpy(le_data.data(), data, static_cast<size_t>(count) * sizeof(float));
  decodeLittleEndian_(le_data.data(), static_cast<Size>(count));
  if (fwrite(le_data.data(), 4, static_cast<size_t>(count), ibd) != static_cast<size_t>(count))
  {
    throwReadError_(ibd_path, "failed to write float32 array to .ibd");
  }
#else
  if (fwrite(data, 4, static_cast<size_t>(count), ibd) != static_cast<size_t>(count))
  {
    throwReadError_(ibd_path, "failed to write float32 array to .ibd");
  }
#endif
}

void ImzMLBinaryIO::writeMzAsFloat32(FILE* ibd,
                                     const std::vector<double>& mz,
                                     const std::string& ibd_path)
{
  if (mz.empty())
  {
    return;
  }
  std::vector<float> tmp(mz.size());
  for (Size i = 0; i < mz.size(); ++i)
  {
    tmp[i] = static_cast<float>(mz[i]);
  }
  writeFloat32Array(ibd, tmp.data(), tmp.size(), ibd_path);
}

void ImzMLBinaryIO::writeFloat64Array(FILE* ibd,
                                      const double* data,
                                      const uint64_t count,
                                      const std::string& ibd_path)
{
  if (!ibd)
  {
    throwReadError_(ibd_path, "invalid .ibd handle (null FILE*) in float64 writer");
  }
  if (count == 0)
  {
    return;
  }
  validateCount_(count, ibd_path, "float64 array write");
#if OPENMS_IS_BIG_ENDIAN
  std::vector<double> le_data(static_cast<size_t>(count));
  std::memcpy(le_data.data(), data, static_cast<size_t>(count) * sizeof(double));
  decodeLittleEndian_(le_data.data(), static_cast<Size>(count));
  if (fwrite(le_data.data(), 8, static_cast<size_t>(count), ibd) != static_cast<size_t>(count))
  {
    throwReadError_(ibd_path, "failed to write float64 array to .ibd");
  }
#else
  if (fwrite(data, 8, static_cast<size_t>(count), ibd) != static_cast<size_t>(count))
  {
    throwReadError_(ibd_path, "failed to write float64 array to .ibd");
  }
#endif
}

void ImzMLBinaryIO::writeMzAsFloat64(FILE* ibd,
                                     const std::vector<double>& mz,
                                     const std::string& ibd_path)
{
  if (mz.empty())
  {
    return;
  }
  writeFloat64Array(ibd, mz.data(), mz.size(), ibd_path);
}

} // namespace OpenMS
