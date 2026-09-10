// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#pragma once

// Standard-library-only utilities owned by the class-test framework (no libOpenMS
// dependency). Numeric formatting is shared with StringUtils; writtenDigits
// mirrors OpenMS::writtenDigits (OpenMS/CONCEPT/Types.h).

#include <OpenMS/CONCEPT/Detail/NumericFormatting.h>

#include <charconv>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace OpenMS
{
  namespace Internal
  {
    namespace ClassTest
    {
      /**
        @brief Decimal digits to print for a value in TEST_REAL_SIMILAR reports.

        Framework copy of OpenMS::writtenDigits; class types convertible to double
        (DataValue, ParamValue) print like a double. Diverges from the old version in
        report precision only (never pass/fail): ParamValue prints at 15 not 6, and
        long/unsigned long use their own digits10 not int's.
      */
      template <typename T>
      constexpr int writtenDigits(const T& = T())
      {
        using C = std::remove_cvref_t<T>;
        if constexpr (std::is_floating_point_v<C>)
        {
          return std::numeric_limits<C>::digits10;
        }
        else if constexpr (std::is_integral_v<C>)
        {
          return std::numeric_limits<C>::digits10;
        }
        else if constexpr (std::is_class_v<C> && std::is_convertible_v<const C&, double>)
        {
          return std::numeric_limits<double>::digits10;
        }
        else
        {
          return 6; // default precision of an ostream (27.4.4.1), like OpenMS::writtenDigits
        }
      }

      /**
        @brief Whether TEST_REAL_SIMILAR may convert a value of this type to double.

        True for floating-point and class types implicitly convertible to double
        (DataValue, ParamValue); anything else is rejected at runtime, as before.
      */
      template <typename T>
      constexpr bool isRealType(const T&)
      {
        using C = std::remove_cvref_t<T>;
        return std::is_floating_point_v<C> || (std::is_class_v<C> && std::is_convertible_v<const C&, double>);
      }

      namespace detail
      {
        /// Remove leading and trailing whitespace (' ', '\\t', '\\n', '\\r'), like StringUtils::trim.
        inline void trim(std::string& s)
        {
          const char* ws = " \t\n\r";
          const size_t b = s.find_first_not_of(ws);
          if (b == std::string::npos)
          {
            s.clear();
            return;
          }
          const size_t e = s.find_last_not_of(ws);
          s = s.substr(b, e - b + 1);
        }

        /// Split on a single character. Mirrors ListUtils::create<std::string>(s, splitter):
        /// an empty input gives an empty result; otherwise empty fields are kept and
        /// fields are NOT trimmed.
        inline std::vector<std::string> split(const std::string& s, char splitter)
        {
          std::vector<std::string> out;
          if (s.empty()) return out;
          size_t begin = 0;
          for (size_t i = 0; i < s.size(); ++i)
          {
            if (s[i] == splitter)
            {
              out.emplace_back(s, begin, i - begin);
              begin = i + 1;
            }
          }
          out.emplace_back(s, begin, s.size() - begin);
          return out;
        }

        /// Join with a separator (display purposes only).
        inline std::string join(const std::vector<std::string>& v, const char* sep)
        {
          std::string out;
          for (size_t i = 0; i < v.size(); ++i)
          {
            if (i) out += sep;
            out += v[i];
          }
          return out;
        }

        ///@name Stringification for TEST_EQUAL(std::string, x). Mirrors the
        /// StringUtils::toStr overloads, so TEST_EQUAL(some_string, 114) compares
        /// against "114" and doubles keep full precision.
        //@{
        inline std::string toString(const std::string& s) { return s; }
        inline std::string toString(std::string_view sv) { return std::string(sv); }
        inline std::string toString(const char* s) { return std::string(s); }
        inline std::string toString(char c) { return std::string(1, c); }
        template <typename T>
        inline std::string toString(const T& v)
        {
          static_assert(std::is_arithmetic_v<T>,
                        "TEST_EQUAL(std::string, x): x must be a string, character or number. "
                        "For other types, stringify explicitly in the test.");
          std::string r;
          if constexpr (std::is_same_v<T, bool>)
          {
            r = v ? "1" : "0";
          }
          else if constexpr (std::is_floating_point_v<T>)
          {
            NumericFormatting::appendNumeric(v, r, std::numeric_limits<T>::digits10, false); // full precision, like toStr(v, true)
          }
          else
          {
            char buf[32];
            auto [p, ec] = std::to_chars(buf, buf + sizeof(buf), v);
            if (ec == std::errc{}) r.append(buf, p);
          }
          return r;
        }
        //@}

        /// Is 'os << t' well-formed? (this header's context + ADL, so vector<T> is
        /// streamable only via an ADL operator, e.g. ListUtilsIO.h for OpenMS types.)
        template <typename T>
        concept Streamable = requires(std::ostream& os, const T& t) { os << t; };

        /// Print a value in a TEST_EQUAL/TEST_NOT_EQUAL failure report: directly if
        /// streamable; ranges as "[e1, e2, ...]" (like ListUtilsIO.h); else a placeholder.
        template <typename T>
        void printValue(std::ostream& os, const T& v)
        {
          using C = std::remove_cvref_t<T>;
          if constexpr (Streamable<C>)
          {
            os << v;
          }
          else if constexpr (std::ranges::input_range<C>)
          {
            os << '[';
            bool first = true;
            for (const auto& e : v)
            {
              if (!first) os << ", ";
              first = false;
              using E = std::remove_cvref_t<decltype(e)>;
              if constexpr (std::is_arithmetic_v<E>)
              {
                os << toString(e); // numbers like ListUtilsIO (via toStr)
              }
              else if constexpr (Streamable<E>)
              {
                os << e;
              }
              else
              {
                os << "<?>";
              }
            }
            os << ']';
          }
          else
          {
            os << "<value of unprintable type - provide operator<< or compare via TEST_TRUE>";
          }
        }

        /// UTF-8 std::string -> std::filesystem::path. Copy of OpenMS::to_path
        /// (OpenMS/SYSTEM/PathUtils.h): on Windows construct from u8string (not the
        /// current code page), falling back to the native page for non-UTF-8 bytes.
        inline std::filesystem::path to_path(const std::string& s)
        {
#ifdef _WIN32
          try
          {
            return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size()));
          }
          catch (const std::system_error&)
          {
            return std::filesystem::path(s);
          }
#else
          return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size()));
#endif
        }
      } // namespace detail

      /**
        @brief Stream wrapper printing a floating point value with full precision.

        The framework's own version of OpenMS::precisionWrapper (OpenMS/CONCEPT/PrecisionWrapper.h):
        printing goes through detail::toString, i.e. the same format as StringUtils::toStr(x, true).
        Used by TEST_FILE_SIMILAR's report.
      */
      template <typename FloatingPointType>
      struct PrecisionWrapped
      {
        FloatingPointType value;
      };

      template <typename FloatingPointType>
      inline PrecisionWrapped<FloatingPointType> precisionWrapper(const FloatingPointType value)
      {
        return PrecisionWrapped<FloatingPointType> {value};
      }

      template <typename FloatingPointType>
      inline std::ostream& operator<<(std::ostream& os, const PrecisionWrapped<FloatingPointType>& p)
      {
        os << detail::toString(p.value);
        return os;
      }
    } // namespace ClassTest
  } // namespace Internal
} // namespace OpenMS
