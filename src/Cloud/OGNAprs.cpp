// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OGNAprs.hpp"

#include "Math/Angle.hpp"
#include "Math/Util.hpp"
#include "Units/System.hpp"
#include "util/CharUtil.hxx"
#include "util/NumberParser.hxx"
#include "util/StringStrip.hxx"

#include <cstdio>

using std::string_view;

/**
 * Parse APRS uncompressed latitude/longitude after the 'h' time-field
 * delimiter, e.g. "4903.50NI00820.62E&".
 */
static bool
ParseAprsLatLon(string_view payload, GeoPoint &location,
                std::size_t *end_pos = nullptr) noexcept
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
  if (!IsDigitASCII(p[i]) || !IsDigitASCII(p[i + 1]))
    return false;
  lat_deg = unsigned(p[i] - '0') * 10u + unsigned(p[i + 1] - '0');
  i += 2;
  if (!IsDigitASCII(p[i]) || !IsDigitASCII(p[i + 1]))
    return false;
  lat_min_int = unsigned(p[i] - '0') * 10u + unsigned(p[i + 1] - '0');
  i += 2;
  if (lat_min_int >= 60)
    return false;
  if (p[i] != '.')
    return false;
  ++i;
  unsigned frac_digits = 0;
  lat_min_frac = 0;
  while (i < p.size() && IsDigitASCII(p[i]) && frac_digits < 2) {
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
  if (!IsDigitASCII(p[i]) || !IsDigitASCII(p[i + 1]) ||
      !IsDigitASCII(p[i + 2]))
    return false;
  lon_deg = unsigned(p[i] - '0') * 100u + unsigned(p[i + 1] - '0') * 10u +
            unsigned(p[i + 2] - '0');
  i += 3;
  if (!IsDigitASCII(p[i]) || !IsDigitASCII(p[i + 1]))
    return false;
  lon_min_int = unsigned(p[i] - '0') * 10u + unsigned(p[i + 1] - '0');
  i += 2;
  if (lon_min_int >= 60)
    return false;
  if (p[i] != '.')
    return false;
  ++i;
  frac_digits = 0;
  lon_min_frac = 0;
  while (i < p.size() && IsDigitASCII(p[i]) && frac_digits < 2) {
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
  if (end_pos != nullptr)
    *end_pos = (h + 1) + i + 1;
  return location.Check();
}

/**
 * APRS course/speed after the position symbol, e.g. "'243/039" or
 * "^085/165".  Course 000 means unknown.
 */
static void
ParseAprsCourseSpeed(string_view payload, std::size_t pos,
                     unsigned &track_deg, bool &track_valid) noexcept
{
  track_valid = false;

  if (pos >= payload.size())
    return;

  ++pos; /* skip APRS symbol code after longitude */

  if (pos + 4 >= payload.size())
    return;

  if (!IsDigitASCII(payload[pos]) || !IsDigitASCII(payload[pos + 1]) ||
      !IsDigitASCII(payload[pos + 2]) || payload[pos + 3] != '/')
    return;

  const unsigned course =
    unsigned(payload[pos] - '0') * 100u +
    unsigned(payload[pos + 1] - '0') * 10u +
    unsigned(payload[pos + 2] - '0');

  if (course == 0 || course > 360)
    return;

  track_deg = course == 360 ? 0u : course;
  track_valid = true;
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
  while (i < payload.size() && IsDigitASCII(payload[i])) {
    feet = feet * 10u + unsigned(payload[i] - '0');
    ++i;
    if (feet > 999999u)
      return false;
  }

  altitude_m = iround(Units::ToSysUnit(double(feet), Unit::FEET));
  return true;
}

/**
 * OGN comment fragment "idXXYYYYYY" (see OGN APRS protocol).
 *
 * XX encodes stealth/no-tracking flags, aircraft type (tttt) and address
 * type (aa).  YYYYYY is the 24-bit sender address.
 */
static bool
ParseOgnIdField(string_view line, uint32_t &id, bool &valid,
                unsigned &aircraft_type, unsigned &address_type) noexcept
{
  valid = false;
  id = 0;
  aircraft_type = 0;
  address_type = 0;

  const auto pos = line.find(" id");
  if (pos == string_view::npos)
    return false;

  const auto hex_start = pos + 3;
  if (hex_start + 8 > line.size())
    return false;

  const string_view hex = line.substr(hex_start, 8);

  uint32_t meta = 0;
  uint32_t addr = 0;
  if (!ParseIntegerTo(hex.substr(0, 2), meta, 16) ||
      !ParseIntegerTo(hex.substr(2, 6), addr, 16))
    return false;

  id = addr & 0xFFFFFFu;
  valid = true;
  address_type = unsigned(meta & 0x03u);
  aircraft_type = unsigned((meta >> 2u) & 0x0Fu);
  return true;
}

static bool
IsOgnGroundStationTocall(string_view line) noexcept
{
  const auto gt = line.find('>');
  if (gt == string_view::npos || gt + 1 >= line.size())
    return false;

  const auto comma = line.find(',', gt + 1);
  const string_view tocall = line.substr(
    gt + 1, comma == string_view::npos ? line.size() - gt - 1 : comma - gt - 1);

  return tocall == "OGNSXR" || tocall == "OGNSDR";
}

bool
IsForwardableOgnTraffic(const OGNAprsParseResult &r,
                        string_view line) noexcept
{
  if (!r.flarm_valid)
    return false;

  if (r.address_type == OGN_ADDRESS_TYPE_TRACKER)
    return false;

  if (IsOgnGroundStationTocall(line))
    return false;

  return true;
}

/**
 * Extract a whitespace-delimited field prefixed by @p tag (e.g. "reg",
 * "fn").
 */
static bool
ExtractTaggedField(string_view line, string_view tag, std::string &dest) noexcept
{
  dest.clear();

  std::size_t pos = 0;
  while (pos < line.size()) {
    pos = line.find(tag, pos);
    if (pos == string_view::npos)
      return false;

    if (pos > 0 && line[pos - 1] != ' ' && line[pos - 1] != '\t') {
      ++pos;
      continue;
    }

    pos += tag.size();
    if (pos >= line.size())
      return false;

    const std::size_t end = line.find_first_of(" \t", pos);
    const std::size_t len = (end == string_view::npos)
      ? line.size() - pos
      : end - pos;
    if (len == 0)
      return false;

    dest.assign(line.data() + pos, len);
    return true;
  }

  return false;
}

static void
ResolveOgnCallsign(string_view line, const OGNAprsParseResult &r,
                   std::string &callsign) noexcept
{
  callsign.clear();

  std::string field;
  if (ExtractTaggedField(line, "reg", field)) {
    callsign = std::move(field);
    return;
  }

  if (ExtractTaggedField(line, "fn", field)) {
    callsign = std::move(field);
    return;
  }

  if (r.flarm_valid) {
    char buf[16];
    const int n = std::snprintf(buf, sizeof(buf), "%06X",
                                unsigned(r.flarm_id & 0xffffffu));
    if (n > 0 && unsigned(n) < sizeof(buf))
      callsign.assign(buf, unsigned(n));
  }
}

OGNAprsParseResult
ParseOGNAprsLine(string_view line) noexcept
{
  OGNAprsParseResult r;

  line = Strip(line);

  if (line.empty() || line[0] == '#')
    return r;

  const auto gt = line.find('>');
  if (gt == string_view::npos || gt == 0)
    return r;

  const auto colon = line.find(':', gt);
  if (colon == string_view::npos || colon + 1 >= line.size())
    return r;

  r.station_id = std::string(line.substr(0, gt));
  const string_view payload = line.substr(colon + 1);

  GeoPoint loc;
  std::size_t pos_end = 0;
  if (!ParseAprsLatLon(payload, loc, &pos_end))
    return r;

  int alt_m = 0;
  const bool altitude_valid = ParseAltitudeFeet(payload, alt_m);

  ParseAprsCourseSpeed(payload, pos_end, r.track_deg, r.track_valid);

  uint32_t fid = 0;
  bool fid_ok = false;
  unsigned aircraft_type = 0;
  unsigned address_type = 0;
  ParseOgnIdField(line, fid, fid_ok, aircraft_type, address_type);

  r.valid = true;
  r.location = loc;
  r.altitude = alt_m;
  r.altitude_valid = altitude_valid;
  r.flarm_id = fid;
  r.flarm_valid = fid_ok;
  r.aircraft_type = aircraft_type;
  r.address_type = address_type;
  ResolveOgnCallsign(line, r, r.callsign);
  return r;
}
