// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Driver.hpp"

#include "FLARM/List.hpp"
#include "FLARM/Id.hpp"
#include "Geo/GeoPoint.hpp"
#include "NMEA/Info.hpp"
#include "Units/System.hpp"
#include "util/CRC16CCITT.hpp"

#include <algorithm>
#include <cstdio>
#include <cmath>

/*
 * References:
 * - Garmin "GDL 90 Data Interface Specification", 560-1058-00 Rev A (2007-06-05)
 *   (a.k.a. the public GDL90 ICD)
 *   - §2.2.1 framing / byte-stuffing: 0x7E flag, 0x7D escape, XOR 0x20
 *   - §2.2.2 message id: MSB reserved, must be zero
 *   - §2.2.3 CRC: CRC-CCITT, table-driven update shown in the document
 *   - §3.1 Heartbeat (msg id 0x00)
 *   - §3.4 Ownship Report (msg id 0x0A), format per §3.5.1
 *   - §3.5 Traffic Report (msg id 0x14), format per §3.5.1
 *
 * Note: Multi-byte fields in report payloads are big-endian unless stated
 * otherwise (ICD §3, "multi-byte fields are transmitted most-significant
 * byte first"). The CRC field itself is little-endian (ICD §2.2.1/§2.2.3).
 */

static constexpr uint8_t GDL90_FLAG = 0x7e;
static constexpr uint8_t GDL90_ESCAPE = 0x7d;
static constexpr uint8_t GDL90_ESCAPE_XOR = 0x20;
static constexpr std::size_t GDL90_MAX_FRAME = 1024;

static constexpr uint8_t
ReadU8(std::span<const uint8_t> s, std::size_t i) noexcept
{
  return i < s.size() ? s[i] : 0;
}

static constexpr uint16_t
ReadBE16(std::span<const uint8_t> s, std::size_t i) noexcept
{
  return (uint16_t(ReadU8(s, i)) << 8) | uint16_t(ReadU8(s, i + 1));
}

static constexpr uint32_t
ReadBE24(std::span<const uint8_t> s, std::size_t i) noexcept
{
  return (uint32_t(ReadU8(s, i)) << 16) |
    (uint32_t(ReadU8(s, i + 1)) << 8) |
    uint32_t(ReadU8(s, i + 2));
}

static constexpr int32_t
SignExtend24(uint32_t value) noexcept
{
  value &= 0x00ffffff;
  if ((value & 0x00800000) != 0)
    return int32_t(value | 0xff000000);
  return int32_t(value);
}

static constexpr double
SemiCircles24ToDegrees(int32_t value) noexcept
{
  /* 24-bit signed semicircle fraction; see GDL90 ICD §3.5.1.3 */
  return double(value) * (180.0 / double(1u << 23));
}

static uint16_t
Gdl90Crc16(std::span<const uint8_t> data) noexcept
{
  /* CRC-CCITT update function per GDL90 ICD §2.2.3 (table-driven). */
  uint16_t crc = 0;

  for (uint8_t b : data)
    crc = crc16ccitt_table[crc >> 8] ^ (crc << 8) ^ b;

  return crc;
}

static bool
UnescapeGdl90(std::span<const uint8_t> src,
              std::vector<uint8_t> &dest) noexcept
{
  dest.clear();
  dest.reserve(src.size());

  for (std::size_t i = 0; i < src.size(); ++i) {
    const uint8_t b = src[i];
    if (b != GDL90_ESCAPE) {
      dest.push_back(b);
      continue;
    }

    if (++i >= src.size())
      return false;

    dest.push_back(src[i] ^ GDL90_ESCAPE_XOR);
  }

  return true;
}

static constexpr bool
IsGdl90MessageId(uint8_t id) noexcept
{
  /* MSB is reserved for future addressing; drop if set (ICD §2.2.2) */
  return (id & 0x80) == 0;
}

static FlarmId
MakeIcaoFlarmId(uint32_t icao) noexcept
{
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%06X", (unsigned)(icao & 0xffffff));
  return FlarmId::Parse(buffer, nullptr);
}

static void
ExtractCallsign(std::span<const uint8_t, 8> src,
                StaticString<10> &dest) noexcept
{
  /* ICD §3.5.1.11: 8 ASCII chars, space is trailing pad */
  char tmp[9];
  for (unsigned i = 0; i < 8; ++i) {
    const uint8_t ch = src[i];
    tmp[i] = ch >= 0x20 && ch < 0x7f ? (char)ch : ' ';
  }
  tmp[8] = 0;

  /* trim trailing spaces */
  unsigned n = 8;
  while (n > 0 && tmp[n - 1] == ' ')
    --n;
  tmp[n] = 0;

  dest.SetASCII(tmp);
  dest.CleanASCII();
}

void
GDL90Device::ParseOwnshipReport(std::span<const uint8_t> payload,
                                NMEAInfo &info) noexcept
{
  /* Ownship Report message payload is 27 bytes (ICD 3.4 / 3.5.1) */
  if (payload.size() < 27)
    return;

  const int32_t lat_raw = SignExtend24(ReadBE24(payload, 4));
  const int32_t lon_raw = SignExtend24(ReadBE24(payload, 7));
  const double lat_deg = SemiCircles24ToDegrees(lat_raw);
  const double lon_deg = SemiCircles24ToDegrees(lon_raw);

  if (!std::isfinite(lat_deg) || !std::isfinite(lon_deg) ||
      lat_deg < -90 || lat_deg > 90 || lon_deg < -180 || lon_deg > 180)
    return;

  info.location = GeoPoint(Angle::Degrees(lon_deg), Angle::Degrees(lat_deg));
  info.location_available.Update(info.clock);

  /* Ownship altitude is pressure altitude (25ft steps, offset -1000ft) */
  const uint16_t alt_word = ReadBE16(payload, 10);
  const uint16_t altitude_code = alt_word >> 4;
  const uint8_t misc = alt_word & 0x0f;
  if (altitude_code != 0xfff) {
    const int altitude_ft = int(altitude_code) * 25 - 1000;
    const double pressure_altitude =
      Units::ToSysUnit(altitude_ft, Unit::FEET);
    info.ProvidePressureAltitude(pressure_altitude);
  }

  /* velocity: 12-bit horizontal (knots), 12-bit vertical (64 fpm) */
  const uint32_t vel = ReadBE24(payload, 13);
  const uint16_t h_code = uint16_t((vel >> 12) & 0x0fff);
  if (h_code != 0x0fff) {
    info.ground_speed = Units::ToSysUnit(h_code, Unit::KNOTS);
    info.ground_speed_available.Update(info.clock);
  }

  const uint8_t track_byte = payload[16];
  const bool track_valid = (misc & 0x03) != 0;
  if (track_valid && track_byte != 0xff) {
    info.track = Angle::Degrees(double(track_byte) * (360.0 / 256.0));
    info.track_available.Update(info.clock);
  }

  /* Basic fix quality (optional but helps status panels) */
  info.gps.fix_quality = FixQuality::GPS;
  info.gps.fix_quality_available.Update(info.clock);
  info.gps.real = true;
}

void
GDL90Device::ParseTrafficReport(std::span<const uint8_t> payload,
                                NMEAInfo &info) noexcept
{
  /* Traffic Report message payload is 27 bytes (ICD 3.5) */
  if (payload.size() < 27)
    return;

  /* byte 0: low nibble = alert status, high nibble = address type
     bytes 1-3: participant address (24-bit) */
  const uint8_t b0 = payload[0];
  const uint8_t address_type = b0 >> 4;

  const uint32_t participant_address = ReadBE24(payload, 1);
  if (participant_address == 0)
    return;

  const FlarmId id = MakeIcaoFlarmId(participant_address);
  if (!id.IsDefined())
    return;

  const int32_t lat_raw = SignExtend24(ReadBE24(payload, 4));
  const int32_t lon_raw = SignExtend24(ReadBE24(payload, 7));
  const double lat_deg = SemiCircles24ToDegrees(lat_raw);
  const double lon_deg = SemiCircles24ToDegrees(lon_raw);

  if (!std::isfinite(lat_deg) || !std::isfinite(lon_deg) ||
      lat_deg < -90 || lat_deg > 90 || lon_deg < -180 || lon_deg > 180)
    return;

  const uint16_t alt_word = ReadBE16(payload, 10);
  const uint16_t altitude_code = alt_word >> 4;
  const uint8_t misc = alt_word & 0x0f;

  bool altitude_available = altitude_code != 0xfff;
  double altitude = 0;
  if (altitude_available) {
    const int altitude_ft = int(altitude_code) * 25 - 1000;
    altitude = Units::ToSysUnit(altitude_ft, Unit::FEET);
  }

  const uint8_t nacp_nic = payload[12];
  (void)nacp_nic; /* not used yet */

  /* velocity: 24 bits, 12-bit horizontal (knots), 12-bit vertical (64 fpm) */
  const uint32_t vel = ReadBE24(payload, 13);
  const uint16_t h_code = uint16_t((vel >> 12) & 0x0fff);
  const uint16_t v_code = uint16_t(vel & 0x0fff);

  bool speed_received = h_code != 0x0fff;
  double speed = 0;
  if (speed_received)
    speed = Units::ToSysUnit(h_code, Unit::KNOTS);

  bool climb_rate_received = v_code != 0x0800;
  double climb_rate = 0;
  if (climb_rate_received) {
    int16_t v_signed = v_code;
    if (v_signed & 0x0800)
      v_signed -= 0x1000;

    /* 64 feet per minute, convert to m/s */
    constexpr double FPM_TO_MS = 0.00508;
    climb_rate = double(v_signed) * 64 * FPM_TO_MS;
  }

  const uint8_t track_byte = payload[16];
  const bool track_valid = (misc & 0x03) != 0;
  bool track_received = track_valid;
  Angle track = Angle::Zero();
  if (track_received && track_byte != 0xff)
    track = Angle::Degrees(double(track_byte) * (360.0 / 256.0));
  else
    track_received = false;

  FlarmTraffic *slot = info.flarm.traffic.FindTraffic(id);
  if (slot == nullptr) {
    slot = info.flarm.traffic.AllocateTraffic();
    if (slot == nullptr)
      return;

    slot->Clear();
    slot->id = id;
    info.flarm.traffic.new_traffic.Update(info.clock);
  }

  info.flarm.traffic.modified.Update(info.clock);
  slot->valid.Update(info.clock);

  if (payload.size() >= 26) {
    const std::span<const uint8_t, 8> callsign{payload.data() + 18, 8};
    ExtractCallsign(callsign, slot->name);
  }

  slot->alarm_level = FlarmTraffic::AlarmType::NONE;
  slot->id_type = FlarmTraffic::IdType::ICAO;

  switch (address_type) {
  case 0: /* ADS-B with ICAO address */
  case 1: /* ADS-B with self-assigned address */
    slot->source = FlarmTraffic::SourceType::ADSB;
    break;

  case 2: /* TIS-B with ICAO address */
  case 3: /* TIS-B with track file ID */
    slot->source = FlarmTraffic::SourceType::TISB;
    break;

  default:
    slot->source = FlarmTraffic::SourceType::ADSB;
    break;
  }

  slot->location = GeoPoint(Angle::Degrees(lon_deg), Angle::Degrees(lat_deg));
  slot->location_available = true;

  if (altitude_available) {
    slot->altitude = altitude;
    slot->altitude_available = true;
  } else {
    slot->altitude = 0;
    slot->altitude_available = false;
  }

  if (track_received) {
    slot->track = track;
    slot->track_received = true;
  } else
    slot->track_received = false;

  if (speed_received) {
    slot->speed = speed;
    slot->speed_received = true;
  } else
    slot->speed_received = false;

  if (climb_rate_received) {
    slot->climb_rate = climb_rate;
    slot->climb_rate_received = true;
  } else
    slot->climb_rate_received = false;

  /* not available / not conveyed by basic Traffic Report */
  slot->relative_north = 0;
  slot->relative_east = 0;
  slot->relative_altitude = 0;
  slot->stealth = false;
  slot->no_track = false;
  slot->rssi_available = false;
  slot->turn_rate_received = false;
  slot->climb_rate_avg30s_available = false;
}

bool
GDL90Device::DataReceived(std::span<const std::byte> s,
                          NMEAInfo &info) noexcept
{
  const auto *p = reinterpret_cast<const uint8_t *>(s.data());
  buffer.insert(buffer.end(), p, p + s.size());

  unescaped.reserve(256);

  while (true) {
    auto start = std::find(buffer.begin(), buffer.end(), GDL90_FLAG);
    if (start == buffer.end()) {
      buffer.clear();
      break;
    }

    buffer.erase(buffer.begin(), start);
    if (buffer.size() < 2)
      break;

    auto end = std::find(buffer.begin() + 1, buffer.end(), GDL90_FLAG);
    if (end == buffer.end()) {
      /* keep current partial frame */
      if (buffer.size() > 4096)
        buffer.erase(buffer.begin(), buffer.end() - 4096);
      break;
    }

    const std::span<const uint8_t> frame{
      buffer.data() + 1, std::size_t(end - buffer.begin() - 1)};

    buffer.erase(buffer.begin(), end + 1);

    if (frame.size() < 4)
      continue;

    if (frame.size() > GDL90_MAX_FRAME)
      continue;

    if (!UnescapeGdl90(frame, unescaped))
      continue;

    if (unescaped.size() < 4)
      continue;

    const uint8_t msg_id = unescaped[0];
    if (!IsGdl90MessageId(msg_id))
      continue;

    if (unescaped.size() < 1 + 2)
      continue;

    const std::size_t payload_len = unescaped.size() - 1 - 2;
    const uint16_t rx_crc = uint16_t(unescaped[unescaped.size() - 2]) |
      (uint16_t(unescaped[unescaped.size() - 1]) << 8);
    const uint16_t calc_crc = Gdl90Crc16({unescaped.data(),
                                          1 + payload_len});
    if (rx_crc != calc_crc)
      continue;

    const std::span<const uint8_t> payload{
      unescaped.data() + 1, payload_len};

    /* Any valid GDL90 frame indicates the device is alive. */
    info.alive.Update(info.clock);

    switch (msg_id) {
    case 0x00: /* Heartbeat */
      break;

    case 0x0a: /* Ownship Report */
      ParseOwnshipReport(payload, info);
      break;

    case 0x14: /* Traffic Report */
      ParseTrafficReport(payload, info);
      break;
    }
  }

  return true;
}

