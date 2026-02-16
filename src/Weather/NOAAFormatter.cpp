// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOAAFormatter.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"
#include "util/StringAPI.hxx"
#include "util/Macros.hpp"

#include <algorithm>

class NOAALineSplitter
{
  const char *start;

public:
  typedef std::pair<const char *, unsigned> Range;

  NOAALineSplitter(const char *_start):start(_start) {}

  bool HasNext() const {
    return start != NULL && start[0] != '\0';
  }

  Range Next() {
    assert(HasNext());

    const char *line_start = start;

    // Search for next line break
    const auto *line_break = StringFind(line_start, '\n');
    if (!line_break) {
      // if no line break was found
      start = NULL;
      return Range(line_start, strlen(line_start));
    }

    unsigned length = line_break - line_start;
    start = line_break + 1;
    return Range(line_start, length);
  }
};

static bool
CheckTitle(const char *title, size_t title_length, const char *check)
{
  if (strlen(check) != title_length)
    return false;

  return std::equal(title, title + title_length, check);
}

static bool
FormatDecodedMETARLine(const char *line, unsigned length,
                       const ParsedMETAR &parsed, std::string &output)
{
  const char *end = line + length;

  const char *colon = (const char *)memchr(line, ':', length);
  if (!colon)
    return false;

  unsigned title_length = colon - line;
  if (title_length == 0)
    return false;

  const char *value = colon + 1;
  while (*value == ' ')
    value++;

  unsigned value_length = end - value;

  if (CheckTitle(line, title_length, "Wind")) {
    StaticString<256> buffer;

    if (!parsed.wind_available) {
      buffer.Format("%s: ", _("Wind"));
      buffer.append({value, value_length});
    } else {
      buffer.Format("%s: %.0f" DEG " %s", _("Wind"),
                    (double)parsed.wind.bearing.Degrees(),
                    FormatUserWindSpeed(parsed.wind.norm).c_str());
    }
    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, "Temperature")) {
    StaticString<256> buffer;

    if (!parsed.temperatures_available) {
      buffer.Format("%s: ", _("Temperature"));
      buffer.append({value, value_length});
    } else {
      char temperature_buffer[16];
      FormatUserTemperature(parsed.temperature, temperature_buffer);

      buffer.Format("%s: %s", _("Temperature"), temperature_buffer);
    }
    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, "Dew Point")) {
    StaticString<256> buffer;

    if (!parsed.temperatures_available) {
      buffer.Format("%s: ", _("Dew Point"));
      buffer.append({value, value_length});
    } else {
      char temperature_buffer[16];
      FormatUserTemperature(parsed.dew_point, temperature_buffer);

      buffer.Format("%s: %s", _("Dew Point"), temperature_buffer);
    }
    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, "Pressure (altimeter)")) {
    StaticString<256> buffer;

    if (!parsed.qnh_available) {
      buffer.Format("%s: ", _("Pressure"));
      buffer.append({value, value_length});
    } else {
      char qnh_buffer[16];
      FormatUserPressure(parsed.qnh, qnh_buffer);

      buffer.Format("%s: %s", _("Pressure"), qnh_buffer);
    }
    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, "Visibility")) {
    StaticString<256> buffer;

    buffer.Format("%s: ", _("Visibility"));
    if (!parsed.visibility_available) {
      buffer.append({value, value_length});
    } else {
      if (parsed.visibility >= 9999)
        buffer.AppendFormat(_("more than %s"),
                            FormatUserDistanceSmart(10000).c_str());
      else
        buffer += FormatUserDistanceSmart(parsed.visibility).c_str();
    }
    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, "Sky conditions")) {
    StaticString<256> buffer;
    buffer.Format("%s: ", _("Sky Conditions"));

    StaticString<64> _value;
    _value.assign({value, value_length});

    buffer += gettext(_value);

    output += buffer;
    output += '\n';
    return true;
  }

  if (CheckTitle(line, title_length, "Weather")) {
    StaticString<256> buffer;
    buffer.Format("%s: ", _("Weather"));

    StaticString<64> _value;
    _value.assign({value, value_length});

    buffer += gettext(_value);

    output += buffer;
    output += '\n';
    return true;
  }

  StaticString<64> title;
  title.assign({line, title_length});

  StaticString<256> buffer;
  buffer.Format("%s: ", gettext(title.c_str()));
  buffer.append({value, value_length});

  output += buffer;
  output += '\n';

  return true;
}

static void
FormatDecodedMETAR(const METAR &metar, const ParsedMETAR &parsed,
                   std::string &output)
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
NOAAFormatter::Format(const NOAAStore::Item &station, std::string &output)
{
  output.reserve(2048);

  if (!station.metar_available) {
    output += _("No METAR available!");
  } else {
    if (station.parsed_metar_available)
      FormatDecodedMETAR(station.metar, station.parsed_metar, output);
    else
      output += station.metar.decoded.c_str();

    output += "\n\n";
    output += station.metar.content.c_str();
  }

  output += "\n\n";

  if (!station.taf_available)
    output += _("No TAF available!");
  else
    output += station.taf.content.c_str();
}
