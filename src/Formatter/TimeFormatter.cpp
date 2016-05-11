/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TimeFormatter.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Math/Util.hpp"
#include "Util/StringCompare.hxx"
#include "Util/StaticString.hxx"

#include <stdio.h>
#include <stdlib.h>

void
FormatISO8601(char *buffer, const BrokenDateTime &stamp)
{
  sprintf(buffer, "%04u-%02u-%02uT%02u:%02u:%02uZ",
          stamp.year, stamp.month, stamp.day,
          stamp.hour, stamp.minute, stamp.second);
}

#ifdef _UNICODE
void
FormatISO8601(TCHAR *buffer, const BrokenDateTime &stamp)
{
  _stprintf(buffer, _T("%04u-%02u-%02uT%02u:%02u:%02uZ"),
            stamp.year, stamp.month, stamp.day,
            stamp.hour, stamp.minute, stamp.second);
}
#endif

void
FormatTime(TCHAR* buffer, double _time)
{
  if (_time < 0) {
    *buffer++ = _T('-');
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSecondOfDayChecked((unsigned)_time);
  _stprintf(buffer, _T("%02u:%02u:%02u"),
            time.hour, time.minute, time.second);
}

void
FormatTimeLong(TCHAR* buffer, double _time)
{
  if (_time < 0) {
    *buffer++ = _T('-');
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSecondOfDayChecked((unsigned)_time);
  _time -= double((int)_time);
  unsigned millisecond = uround(_time * 1000);

  _stprintf(buffer, _T("%02u:%02u:%02u.%03u"),
            time.hour, time.minute, time.second, millisecond);
}

void
FormatSignedTimeHHMM(TCHAR* buffer, int _time)
{
  if (_time < 0) {
    *buffer++ = _T('-');
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSecondOfDayChecked(_time);
  _stprintf(buffer, _T("%02u:%02u"), time.hour, time.minute);
}

void
FormatTimeTwoLines(TCHAR *buffer1, TCHAR *buffer2, int _time)
{
  if (_time >= 24 * 3600) {
    _tcscpy(buffer1, _T(">24h"));
    buffer2[0] = '\0';
    return;
  }
  if (_time <= -24 * 3600) {
    _tcscpy(buffer1, _T("<-24h"));
    buffer2[0] = '\0';
    return;
  }
  if (_time < 0) {
    *buffer1++ = _T('-');
    _time = -_time;
  }

  const BrokenTime time = BrokenTime::FromSecondOfDay(_time);

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
                            unsigned &minutes, unsigned &seconds)
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
FormatTimespanSmart(TCHAR *buffer, int timespan, unsigned max_tokens,
                    const TCHAR *separator)
{
  assert(max_tokens > 0 && max_tokens <= 4);

  unsigned days, hours, minutes, seconds;
  CalculateTimespanComponents(abs(timespan), days, hours, minutes, seconds);

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
  if (timespan < 0) {
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
