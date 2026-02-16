// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "METARParser.hpp"
#include "METAR.hpp"
#include "ParsedMETAR.hpp"
#include "Units/System.hpp"
#include "Atmosphere/Temperature.hpp"
#include "util/CharUtil.hxx"
#include "util/StringAPI.hxx"
#include "util/NumberParser.hpp"

#include <cctype>

namespace METARParser {

bool
ParseLine(const METAR::ContentString &content, ParsedMETAR &parsed);

void
ParseDecoded(const METAR::ContentString &decoded, ParsedMETAR &parsed);

}

class METARLine {
protected:
  char *start, *data, *end;

public:
  /** Constructor. Duplicates the const input to be able to tokenize it. */
  METARLine(const char *line)
    :start(strdup(line)), data(start), end(start + strlen(line))
  {
    // Trim possible = character at the end (End-of-METAR character)
    if (start != end && *(end - 1) == '=') {
      end--;
      *end = '\0';
    }
  }

  /** Destructor. Frees the duplicated input string memory. */
  ~METARLine() {
    free(start);
  }

  /** Returns the next token or NULL if no token is left. (Seperator is ' ') */
  const char *Next() {
    if (data >= end)
      return NULL;

    const char *start = data;

    auto *seperator = StringFind(data, ' ');
    if (seperator != NULL && seperator < end) {
      *seperator = '\0';
      data = seperator + 1;
    } else {
      data = end;
    }

    return start;
  }
};

/** Detects a token with exactly 4 letters */
static bool
DetectICAOCodeToken(const char *token)
{
  if (strlen(token) != 4)
    return false;

  for (unsigned i = 0; i < 4; i++) {
    if ((token[i] >= 'A' && token[i] <= 'Z') ||
        (token[i] >= 'a' && token[i] <= 'z')) {
      // okay
    } else
      return false;
  }

  return true;
}

/** Detects a token with exactly 6 digits and a Z (Zulu = UTC) at the end */
static bool
DetectTimeCodeToken(const char *token)
{
  if (strlen(token) != 7)
    return false;

  return token[6] == 'Z' || token[6] == 'z';
}

static bool
ParseTimeCode(const char *token, ParsedMETAR &parsed)
{
  assert(DetectTimeCodeToken(token));

  char *endptr;
  unsigned time_code = strtod(token, &endptr);
  if (endptr == NULL || endptr == token)
    return false;

  parsed.day_of_month = (int)(time_code / 10000);
  time_code -= parsed.day_of_month * 10000;
  parsed.hour = (int)(time_code / 100);
  time_code -= parsed.hour * 100;
  parsed.minute = time_code;

  return true;
}

/** 
 * Detects a token with exactly 5 digits and MPS or KT at the end. 
 * If the wind direction varies VRB is also valid. 
 */
static bool
DetectWindToken(const char *token)
{
  unsigned length = strlen(token);

  if (length != 8 && length != 7)
    return false;

  if (!StringIsEqualIgnoreCase(token + 5, "MPS") &&
      !StringIsEqualIgnoreCase(token + 5, "KT"))
    return false;

  bool variable = (StringIsEqualIgnoreCase(token, "VRB", 3));

  for (unsigned i = variable ? 3 : 0; i < 5; ++i)
    if (!IsDigitASCII(token[i]))
      return false;

  return true;
}

static bool
ParseWind(const char *token, ParsedMETAR &parsed)
{
  assert(DetectWindToken(token));

  // variable wind directions
  if (StringIsEqualIgnoreCase(token, "VRB", 3))
    // parsing okay but don't provide wind
    return true;

  char *endptr;
  unsigned wind_code = strtod(token, &endptr);
  if (endptr == NULL || endptr == token)
    return false;

  unsigned bearing = (int)(wind_code / 100);
  wind_code -= bearing * 100;

  if (StringIsEqualIgnoreCase(endptr, "MPS"))
    parsed.wind.norm = wind_code;
  else if (StringIsEqualIgnoreCase(endptr, "KT"))
    parsed.wind.norm = Units::ToSysUnit(wind_code, Unit::KNOTS);
  else
    return false;

  parsed.wind.bearing = Angle::Degrees(bearing);
  parsed.wind_available = true;
  return true;
}

/** Detects a CAVOK token */
static bool
DetectCAVOK(const char *token)
{
  return (strlen(token) == 5 && StringIsEqualIgnoreCase(token, "CAVOK"));
}

/** Detects a token with exactly 5 digits */
static bool
DetectVisibilityToken(const char *token)
{
  if (strlen(token) != 4)
    return false;

  for (unsigned i = 0; i < 4; ++i)
    if (!IsDigitASCII(token[i]))
      return false;

  return true;
}

static bool
ParseVisibility(const char *token, ParsedMETAR &parsed)
{
  assert(DetectVisibilityToken(token));

  char *endptr;
  parsed.visibility = strtol(token, &endptr, 10);
  if (endptr == NULL || endptr == token)
    return false;

  parsed.visibility_available = true;
  return true;
}

/** 
 * Detects a token with two pairs of two digits seperated by a slash. 
 * If the temperatures are negative a 'M' is a valid prefix.
 */
static bool
DetectTemperaturesToken(const char *token)
{
  unsigned length = strlen(token);

  bool minus_possible = true;
  bool divider_found = false;

  for (unsigned i = 0; i < length; i++) {
    if (IsDigitASCII(token[i]))
      continue;

    if (token[i] == '/') {
      divider_found = true;
      minus_possible = true;
      continue;
    }

    if (minus_possible && (token[i] == 'M' || token[i] == 'm'))
      continue;

    return false;
  }

  return divider_found;
}

static const char *
ParseTemperature(const char *token, double &temperature)
{
  bool negative = (token[0] == 'M' || token[0] == 'm');
  if (negative)
    token++;

  char *endptr;
  int _temperature = strtod(token, &endptr);
  if (endptr == NULL || endptr == token)
    return NULL;

  if (negative)
    _temperature = -_temperature;

  temperature = CelsiusToKelvin(_temperature);
  return endptr;
}

static bool
ParseTemperatures(const char *token, ParsedMETAR &parsed)
{
  assert(DetectTemperaturesToken(token));

  if ((token = ParseTemperature(token, parsed.temperature)) == NULL)
    return false;

  if (*token != '/')
    return false;

  token++;

  if ((token = ParseTemperature(token, parsed.dew_point)) == NULL)
    return false;

  parsed.temperatures_available = true;
  return true;
}

/** Detects a token beginning with a 'T' and followed by 8 digits */
static bool
DetectAdditionalTemperaturesToken(const char *token)
{
  if (strlen(token) != 9)
    return false;

  if (token[0] != 'T' && token[0] != 't')
    return false;

  for (unsigned i = 1; i < 9; ++i) {
    if (!IsDigitASCII(token[i]))
      return false;
  }

  return true;
}

static bool
ParseAdditionalTemperatures(const char *token, ParsedMETAR &parsed)
{
  assert(DetectAdditionalTemperaturesToken(token));

  // Skip 'T'
  token++;

  char *endptr;
  long temperature_code = strtol(token, &endptr, 10);
  if (endptr == NULL || endptr == token)
    return false;

  int temperature = (int)(temperature_code / 10000);
  int dew_point = temperature_code - temperature * 10000;

  if (temperature >= 1000)
    temperature = -temperature + 1000;

  if (dew_point >= 1000)
    dew_point = -dew_point + 1000;

  parsed.temperature = CelsiusToKelvin(temperature / 10.);
  parsed.dew_point = CelsiusToKelvin(dew_point / 10.);
  parsed.temperatures_available = true;
  return true;
}

/** Detects a token beginning with either 'Q' or 'A' and followed by 4 digits */
static bool
DetectQNHToken(const char *token)
{
  unsigned length = strlen(token);

  // International style
  if (token[0] == 'Q' || token[0] == 'q')
    return length <= 5 && length >= 4;

  // American style
  if (token[0] == 'A' || token[0] == 'a')
    return length == 5;

  return false;
}

static bool
ParseQNH(const char *token, ParsedMETAR &parsed)
{
  assert(DetectQNHToken(token));

  // International style (hPa)
  if (token[0] == 'Q' || token[0] == 'q') {
    token++;

    char *endptr;
    unsigned hpa = strtod(token, &endptr);
    if (endptr == NULL || endptr == token)
      return false;

    parsed.qnh = AtmosphericPressure::HectoPascal(hpa);
    parsed.qnh_available = true;
    return true;
  }

  // American style (inHg)
  if (token[0] == 'A' || token[0] == 'a') {
    token++;

    char *endptr;
    unsigned inch_hg = strtod(token, &endptr);
    if (endptr == NULL || endptr == token)
      return false;

    parsed.qnh = AtmosphericPressure::HectoPascal(Units::ToSysUnit(inch_hg / 100.,
                                                                   Unit::INCH_MERCURY));
    parsed.qnh_available = true;
    return true;
  }

  return false;
}

bool
METARParser::ParseLine(const METAR::ContentString &content, ParsedMETAR &parsed)
{
  // Examples:
  // EDDL 231050Z 31007KT 9999 FEW020 SCT130 23/18 Q1013 NOSIG
  // METAR ETOU 231055Z AUTO 15004KT 9999 FEW130 27/19 A2993 RMK AO2 RAB1038E1048DZB1006E1011 SLP128 P0000 T02710189=
  // METAR KTTN 051853Z 04011KT 1/2SM VCTS SN FZFG BKN003 OVC010 M02/M02 A3006 RMK AO2 TSB40 SLP176 P0002 T10171017=

  METARLine line(content.c_str());
  const char *token;

  // Parse four-letter ICAO code
  while ((token = line.Next()) != NULL) {
    if (DetectICAOCodeToken(token)) {
      parsed.icao_code = token;
      break;
    }
  }

  // Parse day of month, hour and minute
  if ((token = line.Next()) == NULL)
    return false;

  if (!DetectTimeCodeToken(token))
    return false;

  ParseTimeCode(token, parsed);

  // Parse Wind
  while ((token = line.Next()) != NULL) {
    if (DetectWindToken(token)) {
      if (!ParseWind(token, parsed))
        return false;

      break;
    }
  }

  // Parse Visibility
  if ((token = line.Next()) != NULL) {
    if (DetectVisibilityToken(token))
      if (!ParseVisibility(token, parsed))
        return false;

    if (DetectCAVOK(token))
      parsed.cavok = true;
  }

  // Parse Temperatures
  while ((token = line.Next()) != NULL) {
    if (DetectTemperaturesToken(token)) {
      if (!ParseTemperatures(token, parsed))
        return false;

      break;
    }
  }

  // Parse QNH
  while ((token = line.Next()) != NULL) {
    if (DetectQNHToken(token)) {
      if (!ParseQNH(token, parsed))
        return false;

      break;
    }
  }

  // Parse Temperatures
  while ((token = line.Next()) != NULL) {
    if (DetectAdditionalTemperaturesToken(token)) {
      if (!ParseAdditionalTemperatures(token, parsed))
        return false;

      break;
    }
  }

  return true;
}

static bool
ParseLocation(const char *buffer, ParsedMETAR &parsed)
{
  // 51-18N 006-46E
  char *end;
  unsigned lat_deg = ParseUnsigned(buffer, &end, 10);

  if (*end != '-')
    return false;
  end++;

  unsigned lat_min = ParseUnsigned(end, &end, 10);

  unsigned lat_sec = 0;
  if (*end == '-') {
    ++end;
    lat_sec = ParseUnsigned(end, &end, 10);
  }

  bool north;
  if (*end == 'N' || *end == 'n')
    north = true;
  else if (*end == 'S' || *end == 's')
    north = false;
  else
    return false;
  end++;

  while (*end != ' ')
    return false;
  end++;

  unsigned lon_deg = ParseUnsigned(end, &end, 10);

  if (*end != '-')
    return false;
  end++;

  unsigned lon_min = ParseUnsigned(end, &end, 10);

  unsigned lon_sec = 0;
  if (*end == '-') {
    ++end;
    lon_sec = ParseUnsigned(end, &end, 10);
  }

  bool east;
  if (*end == 'E' || *end == 'e')
    east = true;
  else if (*end == 'W' || *end == 'w')
    east = false;
  else
    return false;
  end++;

  GeoPoint location;
  location.latitude = Angle::DMS(lat_deg, lat_min, lat_sec);
  location.longitude = Angle::DMS(lon_deg, lon_min, lon_sec);

  if (!north)
    location.latitude.Flip();

  if (!east)
    location.longitude.Flip();

  parsed.location = location;
  parsed.location_available = true;
  return true;
}

void
METARParser::ParseDecoded(const METAR::ContentString &decoded,
                          ParsedMETAR &parsed)
{
  // Duesseldorf, Germany (EDDL) 51-18N 006-46E 41M
  // Nov 04, 2011 - 07:50 PM EDT / 2011.11.04 2350 UTC

  const char *start = decoded.c_str();
  const char *end = start + strlen(start);
  const auto *opening_brace = StringFind(start, '(');
  const auto *closing_brace = StringFind(start, ')');
  const auto *line_break = StringFind(start, '\n');

  if (line_break == NULL || line_break >= end ||
      opening_brace == NULL || opening_brace >= line_break ||
      closing_brace == NULL || closing_brace >= line_break)
    return;

  while (opening_brace >= start &&
         (*opening_brace == '(' || *opening_brace == ' '))
    opening_brace--;

  unsigned name_length = opening_brace - start + 1;
  if (name_length > 0) {
    parsed.name.assign({start, name_length});
    parsed.name_available = true;
  }

  do
    closing_brace++;
  while (*closing_brace == ' ');

  ParseLocation(closing_brace, parsed);
}

bool
METARParser::Parse(const METAR &metar, ParsedMETAR &parsed)
{
  parsed.Reset();

  if (!metar.decoded.empty())
    ParseDecoded(metar.decoded, parsed);

  return ParseLine(metar.content, parsed);
}
