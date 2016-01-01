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

#include "NOAAFormatter.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"
#include "Util/StringAPI.hxx"
#include "Util/Macros.hpp"

class NOAALineSplitter
{
  const TCHAR *start;

public:
  typedef std::pair<const TCHAR *, unsigned> Range;

  NOAALineSplitter(const TCHAR *_start):start(_start) {}

  bool HasNext() const {
    return start != NULL && start[0] != _T('\0');
  }

  Range Next() {
    assert(HasNext());

    const TCHAR *line_start = start;

    // Search for next line break
    const auto *line_break = StringFind(line_start, _T('\n'));
    if (!line_break) {
      // if no line break was found
      start = NULL;
      return Range(line_start, _tcslen(line_start));
    }

    unsigned length = line_break - line_start;
    start = line_break + 1;
    return Range(line_start, length);
  }
};

static bool
CheckTitle(const TCHAR *title, unsigned title_length, const TCHAR *check)
{
  if (_tcslen(check) != title_length)
    return false;

  return memcmp(title, check, title_length) == 0;
}

static bool
FormatDecodedMETARLine(const TCHAR *line, unsigned length,
                       const ParsedMETAR &parsed, tstring &output)
{
  const TCHAR *end = line + length;

  const TCHAR *colon = (const TCHAR *)memchr(line, _T(':'), length);
  if (!colon)
    return false;

  unsigned title_length = colon - line;
  if (title_length == 0)
    return false;

  const TCHAR *value = colon + 1;
  while (*value == _T(' '))
    value++;

  unsigned value_length = end - value;

  if (CheckTitle(line, title_length, _T("Wind"))) {
    StaticString<256> buffer;

    if (!parsed.wind_available) {
      buffer.Format(_T("%s: "), _("Wind"));
      buffer.append(value, value_length);
    } else {
      TCHAR wind_speed_buffer[16];
      FormatUserWindSpeed(parsed.wind.norm, wind_speed_buffer,
                                 ARRAY_SIZE(wind_speed_buffer));

      buffer.Format(_T("%s: %.0f" DEG " %s"), _("Wind"),
                    (double)parsed.wind.bearing.Degrees(), wind_speed_buffer);
    }
    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, _T("Temperature"))) {
    StaticString<256> buffer;

    if (!parsed.temperatures_available) {
      buffer.Format(_T("%s: "), _("Temperature"));
      buffer.append(value, value_length);
    } else {
      TCHAR temperature_buffer[16];
      FormatUserTemperature(parsed.temperature, temperature_buffer,
                                   ARRAY_SIZE(temperature_buffer));

      buffer.Format(_T("%s: %s"), _("Temperature"), temperature_buffer);
    }
    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, _T("Dew Point"))) {
    StaticString<256> buffer;

    if (!parsed.temperatures_available) {
      buffer.Format(_T("%s: "), _("Dew Point"));
      buffer.append(value, value_length);
    } else {
      TCHAR temperature_buffer[16];
      FormatUserTemperature(parsed.dew_point, temperature_buffer,
                                   ARRAY_SIZE(temperature_buffer));

      buffer.Format(_T("%s: %s"), _("Dew Point"), temperature_buffer);
    }
    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, _T("Pressure (altimeter)"))) {
    StaticString<256> buffer;

    if (!parsed.qnh_available) {
      buffer.Format(_T("%s: "), _("Pressure"));
      buffer.append(value, value_length);
    } else {
      TCHAR qnh_buffer[16];
      FormatUserPressure(parsed.qnh, qnh_buffer, ARRAY_SIZE(qnh_buffer));

      buffer.Format(_T("%s: %s"), _("Pressure"), qnh_buffer);
    }
    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, _T("Visibility"))) {
    StaticString<256> buffer;

    buffer.Format(_T("%s: "), _("Visibility"));
    if (!parsed.visibility_available) {
      buffer.append(value, value_length);
    } else {
      if (parsed.visibility >= 9999)
        buffer.AppendFormat(_("more than %s"),
                            FormatUserDistanceSmart(10000).c_str());
      else
        buffer += FormatUserDistanceSmart(parsed.visibility);
    }
    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, _T("Sky conditions"))) {
    StaticString<256> buffer;
    buffer.Format(_T("%s: "), _("Sky Conditions"));

    StaticString<64> _value;
    _value.assign(value, value_length);

    buffer += gettext(_value);

    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, _T("Weather"))) {
    StaticString<256> buffer;
    buffer.Format(_T("%s: "), _("Weather"));

    StaticString<64> _value;
    _value.assign(value, value_length);

    buffer += gettext(_value);

    output += buffer;
    output += '\n';
    return true;
  }

  StaticString<64> title;
  title.assign(line, title_length);

  StaticString<256> buffer;
  buffer.Format(_T("%s: "), gettext(title.c_str()));
  buffer.append(value, value_length);

  output += buffer;
  output += '\n';

  return true;
}

static void
FormatDecodedMETAR(const METAR &metar, const ParsedMETAR &parsed,
                   tstring &output)
{
  /*
  00 ## Hamburg-Fuhlsbuettel, Germany (EDDH) 53-38N 010-00E 15M ##
  01 ## Dec 14, 2011 - 06:20 PM EST / 2011.12.14 2320 UTC ##
  02 ## Wind: from the SW (220 degrees) at 18 MPH (16 KT):0 ##
  03 ## Visibility: greater than 7 mile(s):0 ##
  04 ## Sky conditions: mostly cloudy ##
  05 ## Temperature: 41 F (5 C) ##
  06 ## Dew Point: 35 F (2 C) ##
  07 ## Relative Humidity: 80% ##
  08 ## Pressure (altimeter): 29.47 in. Hg (0998 hPa) ##
  */

  NOAALineSplitter lines(metar.decoded);
  for (unsigned i = 0; lines.HasNext(); ++i) {
    auto range = lines.Next();

    if (i == 0) {
      // Try to provide a new title line
      if (parsed.name_available) {
        StaticString<256> buffer;
        buffer.Format(_("METAR for %s:"), parsed.name.c_str());
        output += buffer;
        output += '\n';
      } else
        output.append(range.first, range.second);

      output += '\n';

    } else if (i == 1) {
      // ignore second line and continue without line break

    } else {
      if (!FormatDecodedMETARLine(range.first, range.second, parsed, output)) {
        output.append(range.first, range.second);
        output += '\n';
      }
    }
  }
}

void
NOAAFormatter::Format(const NOAAStore::Item &station, tstring &output)
{
  output.reserve(2048);

  if (!station.metar_available) {
    output += _("No METAR available!");
  } else {
    if (station.parsed_metar_available)
      FormatDecodedMETAR(station.metar, station.parsed_metar, output);
    else
      output += station.metar.decoded.c_str();

    output += _T("\n\n");
    output += station.metar.content.c_str();
  }

  output += _T("\n\n");

  if (!station.taf_available)
    output += _("No TAF available!");
  else
    output += station.taf.content.c_str();
}
