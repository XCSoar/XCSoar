// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "Math/Util.hpp"
#include "util/StringCompare.hxx"
#include "util/StaticString.hxx"

#include <stdio.h>
#include <stdlib.h>

void
FormatISO8601(char *buffer, const BrokenDate &date) noexcept
{
  sprintf(buffer, "%04u-%02u-%02u",
          date.year, date.month, date.day);
}

#ifdef _UNICODE
void
FormatISO8601(TCHAR *buffer, const BrokenDate &date) noexcept
{
  _stprintf(buffer, _T("%04u-%02u-%02u"),
            date.year, date.month, date.day);
}
#endif

void
FormatISO8601(char *buffer, const BrokenDateTime &stamp) noexcept
{
  sprintf(buffer, "%04u-%02u-%02uT%02u:%02u:%02uZ",
          stamp.year, stamp.month, stamp.day,
          stamp.hour, stamp.minute, stamp.second);
}

#ifdef _UNICODE
void
FormatISO8601(TCHAR *buffer, const BrokenDateTime &stamp) noexcept
{
  _stprintf(buffer, _T("%04u-%02u-%02uT%02u:%02u:%02uZ"),
            stamp.year, stamp.month, stamp.day,
            stamp.hour, stamp.minute, stamp.second);
}
#endif

void
FormatTime(TCHAR *buffer, FloatDuration _time) noexcept
{
  if (_time.count() < 0) {
    *buffer++ = _T('-');
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSinceMidnightChecked(_time);
  _stprintf(buffer, _T("%02u:%02u:%02u"),
            time.hour, time.minute, time.second);
}

void
FormatTimeLong(TCHAR *buffer, FloatDuration _time) noexcept
{
  if (_time.count() < 0) {
    *buffer++ = _T('-');
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSinceMidnightChecked(_time);

  _time -= FloatDuration{trunc(_time.count())};
  unsigned millisecond = uround(_time.count() * 1000);

  _stprintf(buffer, _T("%02u:%02u:%02u.%03u"),
            time.hour, time.minute, time.second, millisecond);
}

void
FormatSignedTimeHHMM(TCHAR *buffer, std::chrono::seconds _time) noexcept
{
  if (_time.count() < 0) {
    *buffer++ = _T('-');
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSinceMidnightChecked(_time);
  _stprintf(buffer, _T("%02u:%02u"), time.hour, time.minute);
}

void
FormatTimeTwoLines(TCHAR *buffer1, TCHAR *buffer2, std::chrono::seconds _time) noexcept
{
  if (_time >= std::chrono::hours{24}) {
    _tcscpy(buffer1, _T(">24h"));
    buffer2[0] = '\0';
    return;
  }
  if (_time <= -std::chrono::hours{24}) {
    _tcscpy(buffer1, _T("<-24h"));
    buffer2[0] = '\0';
    return;
  }
  if (_time.count() < 0) {
    *buffer1++ = _T('-');
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSinceMidnight(_time);

  if (time.hour > 0) { // hh:mm, ss
    // Set Value
    _stprintf(buffer1, _T("%02u:%02u"), time.hour, time.minute);
    _stprintf(buffer2, _T("%02u"), time.second);
  } else { // mm'ss
    _stprintf(buffer1, _T("%02u'%02u"), time.minute, time.second);
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
FormatTimespanSmart(TCHAR *buffer, std::chrono::seconds timespan,
                    unsigned max_tokens,
                    const TCHAR *separator) noexcept
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
    *buffer = _T('-');
    buffer++;
  }

  *buffer = _T('\0');

  StaticString<16> component_buffer;

  if (show_days) {
    component_buffer.Format(_T("%u days"), days);
    _tcscat(buffer, component_buffer);
  }

  if (show_hours) {
    if (!StringIsEmpty(buffer))
      _tcscat(buffer, separator);

    component_buffer.Format(_T("%u h"), hours);
    _tcscat(buffer, component_buffer);
  }

  if (show_minutes) {
    if (!StringIsEmpty(buffer))
      _tcscat(buffer, separator);

    component_buffer.Format(_T("%u min"), minutes);
    _tcscat(buffer, component_buffer);
  }

  if (show_seconds) {
    if (!StringIsEmpty(buffer))
      _tcscat(buffer, separator);

    component_buffer.Format(_T("%u sec"), seconds);
    _tcscat(buffer, component_buffer);
  }
}
