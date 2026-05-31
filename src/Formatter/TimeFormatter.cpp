// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/Calendar.hxx"
#include "time/Convert.hxx"
#include "Math/Util.hpp"
#include "util/CharUtil.hxx"
#include "util/StringFormat.hpp"
#include "util/StringCompare.hxx"
#include "util/StaticString.hxx"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

void
FormatISO8601(char *buffer, const BrokenDate &date) noexcept
{
  StringFormat(buffer, 11, "%04u-%02u-%02u",
               date.year, date.month, date.day);
}

void
FormatISO8601(char *buffer, const BrokenDateTime &stamp) noexcept
{
  StringFormat(buffer, 21, "%04u-%02u-%02uT%02u:%02u:%02uZ",
               stamp.year, stamp.month, stamp.day,
               stamp.hour, stamp.minute, stamp.second);
}

std::chrono::system_clock::time_point
ParseISO8601Utc(const std::string_view iso_string)
{
  const std::string iso(iso_string);

  struct tm tm = {};
  const char *str = iso.c_str();

  int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
  int consumed = 0;
  const int scanned = sscanf(str, "%d-%d-%dT%d:%d:%d%n",
                           &year, &month, &day, &hour, &min, &sec,
                           &consumed);

  if (scanned < 6)
    throw std::runtime_error("Failed to parse ISO8601 timestamp: " + iso);

  if (year < 0)
    throw std::runtime_error("Invalid ISO8601 timestamp '" + iso +
                             "': year out of range");

  if (month < 1 || month > 12)
    throw std::runtime_error("Invalid ISO8601 timestamp '" + iso +
                             "': month out of range");

  if (day < 1 || day > 31)
    throw std::runtime_error("Invalid ISO8601 timestamp '" + iso +
                             "': day out of range");

  if (static_cast<unsigned>(day) > DaysInMonth(month, year))
    throw std::runtime_error("Invalid ISO8601 timestamp '" + iso +
                           "': impossible date");

  if (hour < 0 || hour > 23)
    throw std::runtime_error("Invalid ISO8601 timestamp '" + iso +
                             "': hour out of range");

  if (min < 0 || min > 59)
    throw std::runtime_error("Invalid ISO8601 timestamp '" + iso +
                             "': minute out of range");

  if (sec < 0 || sec > 59)
    throw std::runtime_error("Invalid ISO8601 timestamp '" + iso +
                             "': second out of range");

  const char *suffix = str + consumed;
  if (*suffix == '.') {
    ++suffix;
    if (!IsDigitASCII(*suffix))
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso +
                               "': non-UTC timezone or extra characters");

    while (IsDigitASCII(*suffix))
      ++suffix;
  }

  if (!(suffix[0] == 'Z' && suffix[1] == '\0'))
    throw std::runtime_error("Invalid ISO8601 timestamp '" + iso +
                             "': UTC suffix 'Z' required");

  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = min;
  tm.tm_sec = sec;
  tm.tm_isdst = 0;

  return TimeGm(tm);
}

void
FormatTime(char *buffer, FloatDuration _time) noexcept
{
  if (_time.count() < 0) {
    *buffer++ = '-';
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSinceMidnightChecked(_time);
  StringFormat(buffer, 9, "%02u:%02u:%02u",
               time.hour, time.minute, time.second);
}

void
FormatTimeLong(char *buffer, FloatDuration _time) noexcept
{
  if (_time.count() < 0) {
    *buffer++ = '-';
    _time = -_time;
  }

  auto time = BrokenTime::FromSinceMidnightChecked(_time);

  _time -= FloatDuration{trunc(_time.count())};
  unsigned millisecond = uround(_time.count() * 1000);

  if (millisecond == 1000) {
    millisecond = 0;
    time = time + std::chrono::seconds{1};
  }

  StringFormat(buffer, 13, "%02u:%02u:%02u.%03u",
               time.hour, time.minute, time.second, millisecond);
}

void
FormatSignedTimeHHMM(char *buffer, std::chrono::seconds _time) noexcept
{
  if (_time.count() < 0) {
    *buffer++ = '-';
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSinceMidnightChecked(_time);
  StringFormat(buffer, 6, "%02u:%02u", time.hour, time.minute);
}

void
FormatTimeTwoLines(char *buffer1, char *buffer2, std::chrono::seconds _time) noexcept
{
  if (_time >= std::chrono::hours{24}) {
    strcpy(buffer1, ">24h");
    buffer2[0] = '\0';
    return;
  }
  if (_time <= -std::chrono::hours{24}) {
    strcpy(buffer1, "<-24h");
    buffer2[0] = '\0';
    return;
  }
  if (_time.count() < 0) {
    *buffer1++ = '-';
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSinceMidnight(_time);

  if (time.hour > 0) { // hh:mm, ss
    // Set Value
    StringFormat(buffer1, 6, "%02u:%02u", time.hour, time.minute);
    StringFormat(buffer2, 3, "%02u", time.second);
  } else { // mm'ss
    StringFormat(buffer1, 6, "%02u'%02u", time.minute, time.second);
    buffer2[0] = '\0';
  }
}

static void
CalculateTimespanComponents(unsigned timespan, unsigned &days, unsigned &hours,
                            unsigned &minutes, unsigned &seconds) noexcept
{
  if (timespan >= 24u * 60u * 60u) {
    days = timespan / (24u * 60u * 60u);
    timespan -= days * (24u * 60u * 60u);
  } else
    days = 0;

  if (timespan >= 60u * 60u) {
    hours = timespan / (60u * 60u);
    timespan -= hours * (60u * 60u);
  } else
    hours = 0;

  if (timespan >= 60u) {
    minutes = timespan / 60u;
    timespan -= minutes * 60u;
  } else
    minutes = 0;

  seconds = timespan;
}

void
FormatTimespanSmart(char *buffer, std::chrono::seconds timespan,
                    unsigned max_tokens,
                    const char *separator) noexcept
{
  assert(max_tokens > 0 && max_tokens <= 4);

  unsigned days, hours, minutes, seconds;
  CalculateTimespanComponents(std::abs(timespan.count()),
                              days, hours, minutes, seconds);

  unsigned token = 0;
  bool show_days = false, show_hours = false;
  bool show_minutes = false, show_seconds = false;

  // Days
  if (days != 0) {
    show_days = true;
    token++;
  }

  // Hours
  if (token < max_tokens) {
    if (hours != 0) {
      show_hours = true;
      token++;
    } else if (token != 0) {
      if (token + 1 < max_tokens && minutes != 0)
        show_hours = true;
      else if (token + 2 < max_tokens && seconds != 0)
        show_hours = true;

      token++;
    }
  }

  // Minutes
  if (token < max_tokens) {
    if (minutes != 0 ) {
      show_minutes = true;
      token++;
    } else if (token != 0) {
      if (token + 1 < max_tokens && seconds != 0)
        show_minutes = true;

      token++;
    }
  }

  // Seconds
  if (token < max_tokens && (seconds != 0 || token == 0))
    show_seconds = true;

  // Output
  if (timespan.count() < 0) {
    *buffer = '-';
    buffer++;
  }

  *buffer = '\0';

  StaticString<16> component_buffer;

  if (show_days) {
    component_buffer.Format("%u days", days);
    strcat(buffer, component_buffer);
  }

  if (show_hours) {
    if (!StringIsEmpty(buffer))
      strcat(buffer, separator);

    component_buffer.Format("%u h", hours);
    strcat(buffer, component_buffer);
  }

  if (show_minutes) {
    if (!StringIsEmpty(buffer))
      strcat(buffer, separator);

    component_buffer.Format("%u min", minutes);
    strcat(buffer, component_buffer);
  }

  if (show_seconds) {
    if (!StringIsEmpty(buffer))
      strcat(buffer, separator);

    component_buffer.Format("%u sec", seconds);
    strcat(buffer, component_buffer);
  }
}
