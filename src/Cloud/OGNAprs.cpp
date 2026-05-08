// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OGNAprs.hpp"

#include "Math/Angle.hpp"

using std::string_view;

static constexpr bool
IsDigit(char ch) noexcept
{
  return ch >= '0' && ch <= '9';
}

/**
 * Parse APRS uncompressed latitude/longitude after the 'h' time-field
 * delimiter, e.g. "4903.50NI00820.62E&".
 */
static bool
ParseAprsLatLon(string_view payload, GeoPoint &location) noexcept
{
  const auto h = payload.find('h');
  if (h == string_view::npos || h + 1 >= payload.size())
    return false;

  string_view p = payload.substr(h + 1);

  /* Latitude: DDMM.MMN or DDMM.MMS */
  if (p.size() < 7)
    return false;

  unsigned lat_deg = 0, lat_min_int = 0, lat_min_frac = 0;
  unsigned i = 0;
  if (!IsDigit(p[i]) || !IsDigit(p[i + 1]))
    return false;
  lat_deg = unsigned(p[i] - '0') * 10u + unsigned(p[i + 1] - '0');
  i += 2;
  if (!IsDigit(p[i]) || !IsDigit(p[i + 1]))
    return false;
  lat_min_int = unsigned(p[i] - '0') * 10u + unsigned(p[i + 1] - '0');
  i += 2;
  if (p[i] != '.')
    return false;
  ++i;
  unsigned frac_digits = 0;
  lat_min_frac = 0;
  while (i < p.size() && IsDigit(p[i]) && frac_digits < 2) {
    lat_min_frac = lat_min_frac * 10u + unsigned(p[i] - '0');
    ++frac_digits;
    ++i;
  }
  if (i >= p.size())
    return false;
  const char lat_hemi = p[i++];
  if (lat_hemi != 'N' && lat_hemi != 'S')
    return false;

  /* APRS inserts a symbol-table / overlay character between lat and lon. */
  if (i < p.size())
    ++i;

  /* Longitude: DDDMM.MME or DDDMM.MMW */
  if (p.size() < i + 8)
    return false;

  unsigned lon_deg = 0, lon_min_int = 0, lon_min_frac = 0;
  if (!IsDigit(p[i]) || !IsDigit(p[i + 1]) || !IsDigit(p[i + 2]))
    return false;
  lon_deg = unsigned(p[i] - '0') * 100u + unsigned(p[i + 1] - '0') * 10u +
            unsigned(p[i + 2] - '0');
  i += 3;
  if (!IsDigit(p[i]) || !IsDigit(p[i + 1]))
    return false;
  lon_min_int = unsigned(p[i] - '0') * 10u + unsigned(p[i + 1] - '0');
  i += 2;
  if (p[i] != '.')
    return false;
  ++i;
  frac_digits = 0;
  lon_min_frac = 0;
  while (i < p.size() && IsDigit(p[i]) && frac_digits < 2) {
    lon_min_frac = lon_min_frac * 10u + unsigned(p[i] - '0');
    ++frac_digits;
    ++i;
  }
  if (i >= p.size())
    return false;
  const char lon_hemi = p[i];
  if (lon_hemi != 'E' && lon_hemi != 'W')
    return false;

  const double lat_min = lat_min_int + lat_min_frac / 100.;
  double lat = lat_deg + lat_min / 60.;
  if (lat_hemi == 'S')
    lat = -lat;

  const double lon_min = lon_min_int + lon_min_frac / 100.;
  double lon = lon_deg + lon_min / 60.;
  if (lon_hemi == 'W')
    lon = -lon;

  location = GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
  return location.IsValid();
}

/**
 * "/A=dddddd" altitude in feet (APRS).
 */
static bool
ParseAltitudeFeet(string_view payload, int &altitude_m) noexcept
{
  const auto a = payload.find("/A=");
  if (a == string_view::npos)
    return false;

  unsigned feet = 0;
  auto i = a + 3;
  while (i < payload.size() && IsDigit(payload[i])) {
    feet = feet * 10u + unsigned(payload[i] - '0');
    ++i;
    if (feet > 999999u)
      return false;
  }

  altitude_m = int(double(feet) * 0.3048 + 0.5);
  return true;
}

/**
 * OGN comment fragment "id06XXXXXX" / "id03XXXXXX" (hex aircraft id).
 */
static bool
ParseOgnIdTag(string_view line, uint32_t &id, bool &valid) noexcept
{
  valid = false;
  id = 0;

  /* Case-sensitive tag as emitted by OGN devices */
  const char *const tags[] = {" id06", " id03", " id21"};
  for (const char *tag : tags) {
    const auto tag_len = std::char_traits<char>::length(tag);
    auto pos = line.find(tag);
    if (pos == string_view::npos)
      continue;

    pos += tag_len;
    unsigned value = 0;
    unsigned digits = 0;
    while (pos < line.size() && digits < 6) {
      const char ch = line[pos];
      unsigned nybble = 16;
      if (ch >= '0' && ch <= '9')
        nybble = unsigned(ch - '0');
      else if (ch >= 'A' && ch <= 'F')
        nybble = 10u + unsigned(ch - 'A');
      else if (ch >= 'a' && ch <= 'f')
        nybble = 10u + unsigned(ch - 'a');
      if (nybble >= 16)
        break;
      value = (value << 4u) | nybble;
      ++digits;
      ++pos;
    }
    if (digits == 6) {
      id = value & 0xFFFFFFu;
      valid = true;
      return true;
    }
  }

  return false;
}

OGNAprsParseResult
ParseOGNAprsLine(string_view line) noexcept
{
  OGNAprsParseResult r;

  while (!line.empty() && (line.front() == '\r' || line.front() == '\n'))
    line.remove_prefix(1);
  while (!line.empty() &&
         (line.back() == '\r' || line.back() == '\n'))
    line.remove_suffix(1);

  if (line.empty() || line[0] == '#')
    return r;

  const auto gt = line.find('>');
  if (gt == string_view::npos || gt == 0)
    return r;

  const auto colon = line.find(':', gt);
  if (colon == string_view::npos || colon + 1 >= line.size())
    return r;

  r.station_id = line.substr(0, gt);
  const string_view payload = line.substr(colon + 1);

  GeoPoint loc;
  if (!ParseAprsLatLon(payload, loc))
    return r;

  int alt_m = 0;
  if (!ParseAltitudeFeet(payload, alt_m))
    /* still accept pressure-alt absent on some frames */
    alt_m = 0;

  uint32_t fid = 0;
  bool fid_ok = false;
  ParseOgnIdTag(line, fid, fid_ok);

  r.valid = true;
  r.location = loc;
  r.altitude = alt_m;
  r.flarm_id = fid;
  r.flarm_valid = fid_ok;
  return r;
}
