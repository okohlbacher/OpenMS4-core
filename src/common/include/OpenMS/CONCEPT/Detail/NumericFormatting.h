// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg, Chris Bielow $
// $Authors: Marc Sturm, Stephan Aiche, Chris Bielow, Timo Sachsenberg $
// --------------------------------------------------------------------------

#pragma once

#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace OpenMS::Internal::NumericFormatting
{
  /// Append a floating-point value as locale-independent decimal text.
  /// NaN is output as "NaN" (uppercase) for backward compatibility, infinities as "inf"/"-inf".
  /// Trailing zeros are trimmed but at least one digit after '.' is kept (matches old karma behavior).
  template <typename T>
  inline void appendNumeric(T value, std::string& target, int precision, bool fixed_format)
  {
    if (std::isnan(value)) { target += "NaN"; return; }
    // Without this, std::to_chars writes "inf", which carries neither '.' nor 'e', so the
    // "keep at least one digit after the decimal point" rule below (there so that 5 prints as 5.0)
    // appended ".0" and produced "inf.0" - a token nothing can read back, which turned any file
    // containing an infinity into one that fails to load. "inf"/"-inf" round-trip through
    // toDouble()/toFloat() as they stand, so only the writing side was ever wrong.
    if (std::isinf(value)) { target += (value < T(0)) ? "-inf" : "inf"; return; }
    char buf[64];
    std::to_chars_result fc;

    // Determine format: use scientific for extreme values or when fixed_format is requested
    // but the value is too small/large for fixed notation
    T abs_val = (value < 0) ? -value : value;
    bool use_scientific = abs_val != T(0) && (abs_val >= T(1e4) || abs_val < T(1e-2));

#if defined(_LIBCPP_VERSION)
    // libc++ implements long-double to_chars by narrowing to double. On Intel macOS
    // that loses both precision and range; streams preserve the original value.
    if constexpr (std::is_same_v<T, long double> &&
                  std::numeric_limits<T>::digits > std::numeric_limits<double>::digits)
    {
      std::ostringstream stream;
      stream.imbue(std::locale::classic());
      stream.precision(use_scientific && !fixed_format ? std::numeric_limits<T>::max_digits10 - 1 : precision);
      stream << (use_scientific ? std::scientific : std::fixed) << value;
      const std::string formatted = stream.str();
      if (formatted.size() > sizeof(buf))
      {
        throw std::length_error("Long-double representation exceeds numeric buffer");
      }
      std::memcpy(buf, formatted.data(), formatted.size());
      fc = {buf + formatted.size(), std::errc{}};
    }
    else
#endif
    if (use_scientific)
    {
      if (fixed_format)
      {
        fc = std::to_chars(buf, buf + sizeof(buf), value, std::chars_format::scientific, 3);
      }
      else
      {
        // Use shortest round-trip representation (no explicit precision). The C++ standard
        // guarantees a unique shortest decimal that round-trips back to the same double, so
        // the result is platform-independent. A fixed precision instead rounds differently
        // across libc++/libstdc++ (the macOS CI divergence this fixes); shortest round-trip
        // reproduces the existing reference-file values without updates.
        fc = std::to_chars(buf, buf + sizeof(buf), value, std::chars_format::scientific);
      }
    }
    else
    {
      fc = std::to_chars(buf, buf + sizeof(buf), value, std::chars_format::fixed, precision);
    }

    if (fc.ec == std::errc{})
    {
      const char* end = fc.ptr;

      // For scientific notation: post-process to match expected format
      // std::to_chars outputs "1.234e+04" but we want "1.234e04"
      const char* e_pos = reinterpret_cast<const char*>(std::memchr(buf, 'e', end - buf));
      if (e_pos)
      {
        const char* e_orig = e_pos; // save before mantissa trimming

        // Trim trailing zeros from mantissa (before 'e')
        const char* dot = reinterpret_cast<const char*>(std::memchr(buf, '.', e_orig - buf));
        if (dot)
        {
          while (e_pos > dot + 1 && *(e_pos - 1) == '0') --e_pos;
          if (e_pos == dot + 1) e_pos = dot + 2; // keep at least one digit after dot
        }
        // Append trimmed mantissa, then 'e'. The shortest round-trip representation can
        // produce an integer mantissa with no decimal point (e.g. "1e+06"); restore the
        // historical "1.0e06" form by ensuring at least one digit after the decimal point.
        target.append(buf, static_cast<size_t>(e_pos - buf));
        if (!dot) target += ".0";
        target += 'e';

        // Fix exponent format: "+04" → "04" (remove '+', keep '-' and zero-padding).
        // std::to_chars uses printf %e style, so the exponent always has >= 2 digits and
        // negative exponents already carry their '-'; only the leading '+' must be removed.
        const char* exp_start = e_orig + 1; // skip 'e'
        if (exp_start < end && *exp_start == '+')
          ++exp_start; // skip '+'
        size_t exp_len = static_cast<size_t>(end - exp_start);
        target.append(exp_start, exp_len);
        return;
      }

      // For fixed notation: trim trailing zeros after decimal point
      const char* dot = reinterpret_cast<const char*>(std::memchr(buf, '.', end - buf));
      if (dot)
      {
        while (end > dot + 1 && *(end - 1) == '0') --end;
        if (end == dot + 1) end = dot + 2; // keep at least one digit after dot
      }
      else if constexpr (std::is_floating_point_v<T>)
      {
        target.append(buf, static_cast<size_t>(end - buf));
        target += ".0";
        return;
      }
      target.append(buf, static_cast<size_t>(end - buf));
    }
    else
    {
      target += std::to_string(static_cast<double>(value));
    }
  }
}
