// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CondorSpectate.hpp"
#include "Device/Port/SpectateFilePort.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/Checksum.hpp"
#include "io/FileReader.hxx"
#include "util/NumberParser.hpp"
#include "util/StringCompare.hxx"
#include "util/StringFormat.hpp"
#include "util/StringStrip.hxx"
#include "util/StringUtil.hpp"
#include "util/TruncateString.hpp"

#include <boost/json/value.hpp>
#include <boost/json/parse.hpp>

#include "Geo/WGS84.hpp"
#include "Math/Angle.hpp"
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

using std::string_view_literals::operator""sv;

namespace {

constexpr std::size_t max_file_size = 1024 * 1024;
constexpr std::size_t max_players = 64;
constexpr std::size_t max_hex_payload = 17;

void
AppendNMEALine(CondorSpectateBuilder::Lines &lines,
               const char *body_without_checksum) noexcept
{
  if (lines.full())
    return;

  CondorSpectateBuilder::LineBuffer &line = lines.append();
  line = body_without_checksum;
  AppendNMEAChecksum(line.buffer());
}

bool
ParseCondorCoord(std::string_view text, double &value) noexcept
{
  text = StripLeft(text);
  if (text.size() < 2)
    return false;

  const char hemi = text.front();
  text.remove_prefix(1);

  char *endptr = nullptr;
  const double v = ParseDouble(text.data(), &endptr);
  if (endptr == text.data())
    return false;

  if (hemi == 'S' || hemi == 'W')
    value = -v;
  else if (hemi == 'N' || hemi == 'E')
    value = v;
  else
    return false;

  return true;
}

bool
JsonDouble(const boost::json::value &v, double &out) noexcept
{
  if (v.is_number()) {
    out = v.to_number<double>();
    return true;
  }

  if (!v.is_string())
    return false;

  char *endptr = nullptr;
  const double d = ParseDouble(v.as_string().c_str(), &endptr);
  if (endptr == v.as_string().c_str())
    return false;

  out = d;
  return true;
}

void
RelativeNE(double ref_lat, double ref_lon,
           double lat, double lon,
           double &north, double &east) noexcept
{
  const double lat_rad = Angle::Degrees(lat).Radians();
  const double d_lat = Angle::Degrees(lat - ref_lat).Radians();
  const double d_lon = Angle::Degrees(lon - ref_lon).Radians();
  north = d_lat * WGS84::EQUATOR_RADIUS;
  east = d_lon * WGS84::EQUATOR_RADIUS * std::cos(lat_rad);
}

unsigned
FlarmHexId(const boost::json::object &player) noexcept
{
  const auto id = player.if_contains("ID");
  if (id == nullptr || !id->is_string())
    return 1;

  const auto s = id->as_string();
  char *endptr = nullptr;
  unsigned long n = std::strtoul(s.c_str(), &endptr, 10);
  if (endptr == s.c_str()) {
    /* fallback hash for non-numeric ids */
    n = static_cast<unsigned long>(std::hash<std::string_view>{}(s));
  }

  n &= 0xFFFFFF;
  return n != 0 ? unsigned(n) : 1;
}

void
FormatFlarmPflaaId(const boost::json::object &player, unsigned hex_id,
                   char *buffer, std::size_t buffer_size) noexcept
{
  StaticString<16> hex;
  hex.Format("%06X", hex_id);

  const auto cn = player.if_contains("CN");
  if (cn != nullptr && cn->is_string() && !cn->as_string().empty()) {
    StringFormat(buffer, buffer_size, "%s!%s",
                 hex.c_str(), cn->as_string().c_str());
    return;
  }

  const auto rn = player.if_contains("RN");
  if (rn != nullptr && rn->is_string() && !rn->as_string().empty()) {
    StringFormat(buffer, buffer_size, "%s!%s",
                 hex.c_str(), rn->as_string().c_str());
    return;
  }

  CopyString(buffer, buffer_size, hex);
}

void
HexPayload(std::string_view text, char *dest, std::size_t dest_size) noexcept
{
  StaticString<max_hex_payload + 1> truncated;
  CopyTruncateString(truncated.buffer(), truncated.capacity(),
                     text.data(), text.size());

  char *p = dest;
  const char *const end = dest + dest_size - 1;
  for (const char ch : truncated) {
    if (p + 2 >= end)
      break;

    const int n = StringFormat(p, end - p, "%02X", (unsigned char)ch);
    if (n <= 0)
      break;

    p += n;
  }

  *p = '\0';
}

void
AppendPflam(CondorSpectateBuilder::Lines &lines, unsigned hex_id,
            const char *msg_type, const char *hex_payload) noexcept
{
  char body[128];
  StringFormat(body, sizeof(body),
               "$PFLAM,U,2,%06X,%s,%s", hex_id, msg_type, hex_payload);
  AppendNMEALine(lines, body);
}

void
AppendPflamForPlayer(CondorSpectateBuilder::Lines &lines,
                     const boost::json::object &player,
                     unsigned hex_id) noexcept
{
  char payload[64];

  const auto first = player.if_contains("firstname");
  const auto last = player.if_contains("lastname");
  if (first != nullptr && first->is_string() &&
      last != nullptr && last->is_string()) {
    StaticString<64> pilot;
    pilot.Format("%s %s", first->as_string().c_str(),
                 last->as_string().c_str());
    StripRight(pilot.buffer());
    if (!pilot.empty()) {
      HexPayload(pilot, payload, sizeof(payload));
      AppendPflam(lines, hex_id, "PNAME", payload);
    }
  }

  const auto cn = player.if_contains("CN");
  if (cn != nullptr && cn->is_string() && !cn->as_string().empty()) {
    HexPayload(cn->as_string(), payload, sizeof(payload));
    AppendPflam(lines, hex_id, "ACALL", payload);
  }

  const auto rn = player.if_contains("RN");
  if (rn != nullptr && rn->is_string() && !rn->as_string().empty()) {
    HexPayload(rn->as_string(), payload, sizeof(payload));
    AppendPflam(lines, hex_id, "AREG", payload);
  }

  const auto plane = player.if_contains("plane");
  if (plane != nullptr && plane->is_string() && !plane->as_string().empty()) {
    HexPayload(plane->as_string(), payload, sizeof(payload));
    AppendPflam(lines, hex_id, "ATYPE", payload);
  }
}

void
AppendPflaa(CondorSpectateBuilder::Lines &lines,
            const boost::json::object &player,
            double rel_n, double rel_e, double rel_v) noexcept
{
  const unsigned hex_id = FlarmHexId(player);

  int track = 0;
  if (const auto heading = player.if_contains("heading");
      heading != nullptr) {
    double track_deg = 0;
    if (JsonDouble(*heading, track_deg))
      track = int(std::fmod(track_deg, 360.0));
  }
  if (track < 0)
    track += 360;

  double speed_kmh = 0;
  if (const auto speed = player.if_contains("speed");
      speed != nullptr) {
    double speed_raw = 0;
    if (JsonDouble(*speed, speed_raw))
      /* Condor spectate speed is already km/h; PFLAA ground speed is km/h. */
      speed_kmh = std::max(0.0, speed_raw);
  }

  double climb = 0;
  if (const auto vario = player.if_contains("vario");
      vario != nullptr)
    JsonDouble(*vario, climb);

  char id_field[32];
  FormatFlarmPflaaId(player, hex_id, id_field, sizeof(id_field));

  char body[160];
  StringFormat(body, sizeof(body),
                 "$PFLAA,0,%d,%d,%d,1,%s,%d,0,%.0f,%.1f,1",
                 int(std::lround(rel_n)), int(std::lround(rel_e)),
                 int(std::lround(rel_v)), id_field, track, speed_kmh, climb);
  AppendNMEALine(lines, body);
}

void
AppendPflau(CondorSpectateBuilder::Lines &lines,
            unsigned traffic_count) noexcept
{
  char body[64];
  StringFormat(body, sizeof(body),
               "$PFLAU,%u,1,2,1,0,,0,,,", traffic_count);
  AppendNMEALine(lines, body);
}

const boost::json::object *
FindOwnShip(const boost::json::array &players,
            std::string_view own_cn) noexcept
{
  if (own_cn.empty())
    return nullptr;

  for (const auto &item : players) {
    if (!item.is_object())
      continue;

    const auto &player = item.as_object();
    const auto cn = player.if_contains("CN");
    if (cn == nullptr || !cn->is_string())
      continue;

    if (StringIsEqual(cn->as_string(), own_cn))
      return &player;
  }

  return nullptr;
}

bool
IsOwnShip(const boost::json::object &player,
          const boost::json::object *own) noexcept
{
  if (own == nullptr)
    return false;

  const auto id = player.if_contains("ID");
  const auto own_id = own->if_contains("ID");
  if (id == nullptr || own_id == nullptr)
    return false;

  if (id->is_string() && own_id->is_string())
    return StringIsEqual(id->as_string(), own_id->as_string());

  return *id == *own_id;
}

bool
PlayerCoords(const boost::json::object &player,
             double &lat, double &lon, double &alt) noexcept
{
  const auto lat_v = player.if_contains("latitude");
  const auto lon_v = player.if_contains("longitude");
  if (lat_v == nullptr || lon_v == nullptr ||
      !lat_v->is_string() || !lon_v->is_string())
    return false;

  if (!ParseCondorCoord(lat_v->as_string(), lat) ||
      !ParseCondorCoord(lon_v->as_string(), lon))
    return false;

  alt = 0;
  if (const auto alt_v = player.if_contains("altitude");
      alt_v != nullptr)
    JsonDouble(*alt_v, alt);

  return true;
}

} // namespace

bool
CondorSpectateBuilder::Build(Path path, const char *own_cn,
                               Lines &lines,
                               const CondorSpectateReference *live_ref) noexcept
{
  lines.clear();

  try {
    FileReader reader(path);
    const auto size = reader.GetSize();
    if (size == 0 || size > max_file_size) {
      AppendPflau(lines, 0);
      return true;
    }

    std::string content(size, '\0');
    const std::size_t nbytes = reader.Read(
      std::span{reinterpret_cast<std::byte *>(content.data()), content.size()});
    content.resize(nbytes);

    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF)
      content.erase(0, 3);

    const boost::json::value jv = boost::json::parse(content);
    boost::json::array players;
    if (jv.is_array())
      players = jv.as_array();
    else if (jv.is_object())
      players.emplace_back(jv);
    else
      return false;

    if (players.empty()) {
      AppendPflau(lines, 0);
      return true;
    }

    const std::string_view own_cn_view{own_cn != nullptr ? own_cn : ""};
    const boost::json::object *own = FindOwnShip(players, own_cn_view);

    double ref_lat = 0, ref_lon = 0, ref_alt = 0;
    if (live_ref != nullptr && live_ref->defined) {
      ref_lat = live_ref->latitude;
      ref_lon = live_ref->longitude;
      ref_alt = live_ref->altitude;
    } else if (own != nullptr) {
      if (!PlayerCoords(*own, ref_lat, ref_lon, ref_alt))
        return false;
    } else {
      bool found_ref = false;
      for (const auto &item : players) {
        if (!item.is_object())
          continue;

        if (PlayerCoords(item.as_object(), ref_lat, ref_lon, ref_alt)) {
          found_ref = true;
          break;
        }
      }

      if (!found_ref)
        return false;
    }

    unsigned traffic_count = 0;
    for (const auto &item : players) {
      if (!item.is_object())
        continue;

      const auto &player = item.as_object();
      if (IsOwnShip(player, own))
        continue;

      double lat, lon, alt;
      if (!PlayerCoords(player, lat, lon, alt))
        continue;

      double rel_n, rel_e;
      RelativeNE(ref_lat, ref_lon, lat, lon, rel_n, rel_e);
      const double rel_v = alt - ref_alt;

      const unsigned hex_id = FlarmHexId(player);
      AppendPflamForPlayer(lines, player, hex_id);
      AppendPflaa(lines, player, rel_n, rel_e, rel_v);
      ++traffic_count;

      if (traffic_count >= max_players)
        break;
    }

    AppendPflau(lines, traffic_count);
    return true;
  } catch (...) {
    AppendPflau(lines, 0);
    return true;
  }
}

void
CondorSpectateDevice::OnCalculatedUpdate(const MoreData &basic,
                                         [[maybe_unused]] const DerivedInfo &calculated)
{
  if (!basic.location_available || !basic.location.IsValid()) {
    live_ref.defined = false;
    return;
  }

  live_ref.latitude = basic.location.latitude.Degrees();
  live_ref.longitude = basic.location.longitude.Degrees();
  if (basic.gps_altitude_available)
    live_ref.altitude = basic.gps_altitude;
  else if (basic.baro_altitude_available)
    live_ref.altitude = basic.baro_altitude;
  else
    live_ref.altitude = 0;
  live_ref.defined = true;
}

static Device *
CondorSpectateCreateOnPort([[maybe_unused]] const DeviceConfig &config,
                           Port &com_port)
{
  auto *device = new CondorSpectateDevice();
  static_cast<SpectateFilePort &>(com_port).SetDevice(device);
  return device;
}

const struct DeviceRegister condor_spectate_driver = {
  "CondorSpectate",
  "Condor 3 Spectate multiplayer traffic (mockup)",
  0,
  CondorSpectateCreateOnPort,
};
