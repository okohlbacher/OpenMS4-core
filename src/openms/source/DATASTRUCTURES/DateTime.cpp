// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Nico Pfeifer $
// --------------------------------------------------------------------------

#include <OpenMS/DATASTRUCTURES/DateTime.h>

#include <OpenMS/DATASTRUCTURES/StringUtils.h>
#include <OpenMS/CONCEPT/Exception.h>

#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <tuple>

using namespace std;

namespace OpenMS
{
  // helper: validate date fields
  static bool isValidDate_(int year, int month, int day)
  {
    if (month < 1 || month > 12 || day < 1 || year < 1)
    {
      return false;
    }
    static const int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max_day = days_in_month[month];
    if (month == 2)
    {
      bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
      if (leap) max_day = 29;
    }
    return day <= max_day;
  }

  // helper: validate time fields
  static bool isValidTime_(int hour, int minute, int second)
  {
    return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59;
  }

  // helper: days since 1970-01-01 from a proleptic Gregorian date, and back.
  // Plain integer arithmetic: timegm/_mkgmtime reject years before the libc epoch on some
  // hosts -- macOS returns time_t(-1) for every year up to 1899 -- and the failure is
  // indistinguishable from a valid result, so a valid early date came back as 1969-12-31.
  static long long daysFromCivil_(int year, int month, int day)
  {
    year -= month <= 2;
    const long long era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);                  // [0, 399]
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1; // [0, 365]
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                    // [0, 146096]
    return era * 146097LL + static_cast<long long>(doe) - 719468;
  }

  static void civilFromDays_(long long days, int& year, int& month, int& day)
  {
    days += 719468;
    const long long era = (days >= 0 ? days : days - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(days - era * 146097);            // [0, 146096]
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);               // [0, 365]
    const unsigned mp = (5 * doy + 2) / 153;                                    // [0, 11]
    day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);                       // [1, 31]
    month = static_cast<int>(mp + (mp < 10 ? 3 : -9));                          // [1, 12]
    year = static_cast<int>(static_cast<long long>(yoe) + era * 400 + (month <= 2));
  }

  // helper: add seconds to naive calendar fields
  static void addSecsToFields_(int& year, int& month, int& day,
                               int& hour, int& minute, int& second, int secs)
  {
    long long total = daysFromCivil_(year, month, day) * 86400LL
                      + hour * 3600LL + minute * 60LL + second + secs;
    long long days = total / 86400;
    long long rest = total % 86400;
    if (rest < 0)
    {
      rest += 86400;
      --days;
    }
    civilFromDays_(days, year, month, day);
    hour = static_cast<int>(rest / 3600);
    minute = static_cast<int>((rest % 3600) / 60);
    second = static_cast<int>(rest % 60);
  }

  // helper: milliseconds from the fractional part of a timestamp, read from the text itself
  static int parseMillis_(const std::string& date)
  {
    const auto dot = date.find('.');
    if (dot == std::string::npos) return 0;
    int millisecond = 0, digits = 0;
    for (size_t i = dot + 1; i < date.size() && std::isdigit(static_cast<unsigned char>(date[i])); ++i, ++digits)
    {
      if (digits < 3) millisecond = millisecond * 10 + (date[i] - '0');
    }
    for (int i = digits; i < 3; ++i) millisecond *= 10;
    return millisecond;
  }

  // helper: parse 3-letter month abbreviation to month number (1-12), returns 0 on failure
  static int parseMonthAbbrev_(const char* s)
  {
    static const char* months[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    for (int i = 0; i < 12; ++i)
    {
      if (strncmp(s, months[i], 3) == 0)
      {
        return i + 1;
      }
    }
    return 0;
  }

  DateTime::DateTime() = default;

  bool DateTime::operator==(const DateTime& rhs) const
  {
    return std::tie(fields_.year, fields_.month, fields_.day,
                    fields_.hour, fields_.minute, fields_.second, fields_.millisecond, fields_.valid)
        == std::tie(rhs.fields_.year, rhs.fields_.month, rhs.fields_.day,
                    rhs.fields_.hour, rhs.fields_.minute, rhs.fields_.second, rhs.fields_.millisecond, rhs.fields_.valid);
  }

  bool DateTime::operator!=(const DateTime& rhs) const
  {
    return !(*this == rhs);
  }

  bool DateTime::operator<(const DateTime& rhs) const
  {
    return std::tie(fields_.year, fields_.month, fields_.day,
                    fields_.hour, fields_.minute, fields_.second, fields_.millisecond)
         < std::tie(rhs.fields_.year, rhs.fields_.month, rhs.fields_.day,
                    rhs.fields_.hour, rhs.fields_.minute, rhs.fields_.second, rhs.fields_.millisecond);
  }

  bool DateTime::isValid() const
  {
    return fields_.valid;
  }

  std::string DateTime::toString(const std::string& format) const
  {
    if (!fields_.valid) return std::string();

    char buf[64];

    if (format == "yyyy-MM-ddThh:mm:ss")
    {
      snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
               fields_.year, fields_.month, fields_.day,
               fields_.hour, fields_.minute, fields_.second);
    }
    else if (format == "yyyy-MM-ddThh:mm:ss.zzz")
    {
      snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
               fields_.year, fields_.month, fields_.day,
               fields_.hour, fields_.minute, fields_.second, fields_.millisecond);
    }
    else if (format == "yyyy-MM-dd hh:mm:ss")
    {
      snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
               fields_.year, fields_.month, fields_.day,
               fields_.hour, fields_.minute, fields_.second);
    }
    else if (format == "yyyy-MM-dd+hh:mm")
    {
      snprintf(buf, sizeof(buf), "%04d-%02d-%02d+%02d:%02d",
               fields_.year, fields_.month, fields_.day,
               fields_.hour, fields_.minute);
    }
    else if (format == "yyyy-MM-ddThh:mm:ssZ")
    {
      snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
               fields_.year, fields_.month, fields_.day,
               fields_.hour, fields_.minute, fields_.second);
    }
    else if (format == "yyyy-MM-dd")
    {
      snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
               fields_.year, fields_.month, fields_.day);
    }
    else if (format == "hh:mm:ss")
    {
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
               fields_.hour, fields_.minute, fields_.second);
    }
    else
    {
      throw Exception::InvalidValue(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
                                    "Unknown DateTime format string", format);
    }

    return std::string(buf);
  }

  void DateTime::set(const std::string& date)
  {
    clear();

    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, millisecond = 0;
    bool parsed = false;

    if (StringUtils::has(date, '.') && !StringUtils::has(date, 'T'))
    {
      // German format: dd.MM.yyyy hh:mm:ss
      if (sscanf(date.c_str(), "%d.%d.%d %d:%d:%d", &day, &month, &year, &hour, &minute, &second) == 6)
      {
        parsed = true;
      }
    }
    else if (StringUtils::has(date, '/'))
    {
      // US format: MM/dd/yyyy hh:mm:ss
      if (sscanf(date.c_str(), "%d/%d/%d %d:%d:%d", &month, &day, &year, &hour, &minute, &second) == 6)
      {
        parsed = true;
      }
    }
    else if (StringUtils::has(date, '-'))
    {
      if (StringUtils::has(date, 'T'))
      {
        if (StringUtils::has(date, '+'))
        {
          // ISO with timezone suffix -- strip at '+'
          std::string stripped = StringUtils::prefix(date, '+');
          if (StringUtils::has(stripped, '.'))
          {
            // with milliseconds
            if (sscanf(stripped.c_str(), "%d-%d-%dT%d:%d:%d.%d", &year, &month, &day, &hour, &minute, &second, &millisecond) == 7)
            {
              millisecond = parseMillis_(stripped);
              parsed = true;
            }
          }
          else
          {
            if (sscanf(stripped.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6)
            {
              parsed = true;
            }
          }
        }
        else if (StringUtils::has(date, '.'))
        {
          // ISO 8601 with milliseconds, no timezone: yyyy-MM-ddThh:mm:ss.zzz
          if (sscanf(date.c_str(), "%d-%d-%dT%d:%d:%d.%d", &year, &month, &day, &hour, &minute, &second, &millisecond) == 7)
          {
            millisecond = parseMillis_(date);
            parsed = true;
          }
        }
        else
        {
          // ISO 8601: yyyy-MM-ddThh:mm:ss
          if (sscanf(date.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6)
          {
            parsed = true;
          }
        }
      }
      else if (StringUtils::has(date, 'Z'))
      {
        // Legacy literal: yyyy-MM-ddZ  (Z is discarded, NOT UTC)
        if (sscanf(date.c_str(), "%d-%d-%dZ", &year, &month, &day) == 3)
        {
          // Verify there's nothing unexpected after Z
          // Check: the format is exactly "yyyy-MM-ddZ" -- if there's more after Z, it's invalid
          // Find Z position
          auto zpos = date.find('Z');
          if (zpos != std::string::npos && zpos + 1 == date.size())
          {
            parsed = true;
          }
        }
      }
      else if (StringUtils::has(date, '+'))
      {
        // Legacy literal: yyyy-MM-dd+hh:mm  (+ is separator, NOT timezone)
        if (sscanf(date.c_str(), "%d-%d-%d+%d:%d", &year, &month, &day, &hour, &minute) == 5)
        {
          parsed = true;
        }
      }
      else
      {
        // ISO with space: yyyy-MM-dd hh:mm:ss
        if (sscanf(date.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6)
        {
          parsed = true;
        }
      }
    }

    // Fallback: Legacy protXML format "ddd MMM d YYYY" e.g. "Tue Jul 13 2004"
    // or "ddd MMM dd hh:mm:ss yyyy" e.g. "Thu Dec 14 11:45:26 2006"
    if (!parsed)
    {
      // Try: "Xxx Xxx dd hh:mm:ss yyyy"
      char day_name[4] = {};
      char month_name[4] = {};
      int d2 = 0, h2 = 0, m2 = 0, s2 = 0, y2 = 0;
      if (sscanf(date.c_str(), "%3s %3s %d %d:%d:%d %d", day_name, month_name, &d2, &h2, &m2, &s2, &y2) == 7)
      {
        int mon = parseMonthAbbrev_(month_name);
        if (mon > 0)
        {
          year = y2; month = mon; day = d2;
          hour = h2; minute = m2; second = s2;
          parsed = true;
        }
      }
      else if (sscanf(date.c_str(), "%3s %3s %d %d", day_name, month_name, &d2, &y2) == 4)
      {
        int mon = parseMonthAbbrev_(month_name);
        if (mon > 0)
        {
          year = y2; month = mon; day = d2;
          parsed = true;
        }
      }
    }

    if (!parsed || !isValidDate_(year, month, day) || !isValidTime_(hour, minute, second))
    {
      throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, date, "Invalid date time string");
    }

    fields_.year = year;
    fields_.month = month;
    fields_.day = day;
    fields_.hour = hour;
    fields_.minute = minute;
    fields_.second = second;
    fields_.millisecond = millisecond;
    fields_.valid = true;
  }

  void DateTime::set(UInt month, UInt day, UInt year, UInt hour, UInt minute, UInt second)
  {
    if (!isValidDate_((int)year, (int)month, (int)day) || !isValidTime_((int)hour, (int)minute, (int)second))
    {
      std::string date_time =StringUtils::toStr(year) + "-" + StringUtils::toStr(month) + "-" + StringUtils::toStr(day)
                         + " " + StringUtils::toStr(hour) + ":" + StringUtils::toStr(minute) + ":" + StringUtils::toStr(second);
      throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, date_time, "Invalid date time");
    }

    fields_.year = (int)year;
    fields_.month = (int)month;
    fields_.day = (int)day;
    fields_.hour = (int)hour;
    fields_.minute = (int)minute;
    fields_.second = (int)second;
    fields_.millisecond = 0;
    fields_.valid = true;
  }

  DateTime DateTime::now()
  {
    auto tp = chrono::system_clock::now();
    time_t tt = chrono::system_clock::to_time_t(tp);
    struct tm local{};
#ifdef _WIN32
    localtime_s(&local, &tt);
#else
    localtime_r(&tt, &local);
#endif

    DateTime d;
    d.fields_.year = local.tm_year + 1900;
    d.fields_.month = local.tm_mon + 1;
    d.fields_.day = local.tm_mday;
    d.fields_.hour = local.tm_hour;
    d.fields_.minute = local.tm_min;
    d.fields_.second = local.tm_sec;
    d.fields_.millisecond = 0;
    d.fields_.valid = true;
    return d;
  }

  DateTime DateTime::nowUTC()
  {
    auto tp = chrono::system_clock::now();
    time_t tt = chrono::system_clock::to_time_t(tp);
    struct tm utc{};
#ifdef _WIN32
    _gmtime64_s(&utc, &tt);
#else
    gmtime_r(&tt, &utc);
#endif

    DateTime d;
    d.fields_.year = utc.tm_year + 1900;
    d.fields_.month = utc.tm_mon + 1;
    d.fields_.day = utc.tm_mday;
    d.fields_.hour = utc.tm_hour;
    d.fields_.minute = utc.tm_min;
    d.fields_.second = utc.tm_sec;
    d.fields_.millisecond = 0;
    d.fields_.valid = true;
    return d;
  }

  std::string DateTime::get() const
  {
    if (fields_.valid)
    {
      return toString("yyyy-MM-dd hh:mm:ss");
    }
    return "0000-00-00 00:00:00";
  }

  void DateTime::get(UInt& month, UInt& day, UInt& year,
                     UInt& hour, UInt& minute, UInt& second) const
  {
    year = fields_.year;
    month = fields_.month;
    day = fields_.day;
    hour = fields_.hour;
    minute = fields_.minute;
    second = fields_.second;
  }

  void DateTime::clear()
  {
    fields_ = Fields{};
  }

  void DateTime::setDate(const std::string& date)
  {
    int year = 0, month = 0, day = 0;
    bool parsed = false;

    if (StringUtils::has(date, '-'))
    {
      if (sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day) == 3)
      {
        parsed = true;
      }
    }
    else if (StringUtils::has(date, '.'))
    {
      if (sscanf(date.c_str(), "%d.%d.%d", &day, &month, &year) == 3)
      {
        parsed = true;
      }
    }
    else if (StringUtils::has(date, '/'))
    {
      if (sscanf(date.c_str(), "%d/%d/%d", &month, &day, &year) == 3)
      {
        parsed = true;
      }
    }

    if (!parsed || !isValidDate_(year, month, day))
    {
      throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, date, "Could not set date");
    }

    fields_.year = year;
    fields_.month = month;
    fields_.day = day;
    fields_.valid = true;
  }

  void DateTime::setTime(const std::string& time)
  {
    int hour = 0, minute = 0, second = 0;

    if (sscanf(time.c_str(), "%d:%d:%d", &hour, &minute, &second) != 3 || !isValidTime_(hour, minute, second))
    {
      throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, time, "Could not set time");
    }

    fields_.hour = hour;
    fields_.minute = minute;
    fields_.second = second;
    // If we're setting time on a previously null DateTime, mark as valid
    // (matches Qt behavior where setTime on a null QDateTime makes it valid with date 0)
    fields_.valid = true;
  }

  void DateTime::setDate(UInt month, UInt day, UInt year)
  {
    if (!isValidDate_((int)year, (int)month, (int)day))
    {
      throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
                                  StringUtils::toStr(year) + "-" + StringUtils::toStr(month) + "-" + StringUtils::toStr(day), "Could not set date");
    }

    fields_.year = (int)year;
    fields_.month = (int)month;
    fields_.day = (int)day;
    fields_.valid = true;
  }

  void DateTime::setTime(UInt hour, UInt minute, UInt second)
  {
    if (!isValidTime_((int)hour, (int)minute, (int)second))
    {
      throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
                                  StringUtils::toStr(hour) + ":" + StringUtils::toStr(minute) + ":" + StringUtils::toStr(second), "Could not set time");
    }

    fields_.hour = (int)hour;
    fields_.minute = (int)minute;
    fields_.second = (int)second;
    fields_.valid = true;
  }

  void DateTime::getDate(UInt& month, UInt& day, UInt& year) const
  {
    month = fields_.month;
    day = fields_.day;
    year = fields_.year;
  }

  std::string DateTime::getDate() const
  {
    if (fields_.valid)
    {
      return toString("yyyy-MM-dd");
    }
    return "0000-00-00";
  }

  void DateTime::getTime(UInt& hour, UInt& minute, UInt& second) const
  {
    hour = fields_.hour;
    minute = fields_.minute;
    second = fields_.second;
  }

  std::string DateTime::getTime() const
  {
    if (fields_.valid)
    {
      return toString("hh:mm:ss");
    }
    return "00:00:00";
  }

  DateTime& DateTime::addSecs(int s)
  {
    addSecsToFields_(fields_.year, fields_.month, fields_.day,
                     fields_.hour, fields_.minute, fields_.second, s);
    return *this;
  }

  bool DateTime::isNull() const
  {
    return !fields_.valid;
  }

  // static
  DateTime DateTime::fromString(const std::string& date, const std::string& format)
  {
    DateTime d;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, millisecond = 0;
    int n = 0;

    if (format == "yyyy-MM-ddThh:mm:ss")
    {
      n = sscanf(date.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second);
      if (n != 6) return d; // return invalid
    }
    else if (format == "yyyy-MM-ddThh:mm:ss.zzz")
    {
      n = sscanf(date.c_str(), "%d-%d-%dT%d:%d:%d.%d", &year, &month, &day, &hour, &minute, &second, &millisecond);
      if (n != 7) return d;
      millisecond = parseMillis_(date);
    }
    else if (format == "yyyy-MM-dd hh:mm:ss")
    {
      n = sscanf(date.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
      if (n != 6) return d;
    }
    else if (format == "yyyy-MM-dd+hh:mm")
    {
      n = sscanf(date.c_str(), "%d-%d-%d+%d:%d", &year, &month, &day, &hour, &minute);
      if (n != 5) return d;
    }
    else if (format == "yyyy-MM-ddThh:mm:ssZ")
    {
      n = sscanf(date.c_str(), "%d-%d-%dT%d:%d:%dZ", &year, &month, &day, &hour, &minute, &second);
      if (n != 6) return d;
    }
    else if (format == "yyyy-MM-dd")
    {
      n = sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day);
      if (n != 3) return d;
    }
    else if (format == "hh:mm:ss")
    {
      n = sscanf(date.c_str(), "%d:%d:%d", &hour, &minute, &second);
      if (n != 3) return d;
    }
    else
    {
      return d; // return invalid for unknown format
    }

    if (!isValidDate_(year, month, day) && format != "hh:mm:ss")
    {
      return d;
    }
    if (!isValidTime_(hour, minute, second))
    {
      return d;
    }

    d.fields_.year = year;
    d.fields_.month = month;
    d.fields_.day = day;
    d.fields_.hour = hour;
    d.fields_.minute = minute;
    d.fields_.second = second;
    d.fields_.millisecond = millisecond;
    d.fields_.valid = true;
    return d;
  }

} // namespace OpenMS
