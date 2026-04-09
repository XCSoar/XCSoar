// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Driver.hpp"

#include "FLARM/List.hpp"
#include "FLARM/Id.hpp"
#include "Geo/Geoid.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "NMEA/Info.hpp"
#include "Profile/Keys.hpp"
#include "Profile/ProfileMap.hpp"
#include "Units/System.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/CRC16CCITT.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>

/*
 * GDL90 driver — see Protocol.md in this directory for framing, endianness,
 * and payload byte maps.
 *
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
static constexpr std::size_t GDL90_BUFFER_TRIM_SIZE = 4096;
static constexpr std::size_t GDL90_UNESCAPED_INITIAL_CAPACITY = 256;

/* Message identifiers (ICD §2.2.2, §3) */
static constexpr uint8_t GDL90_MSG_HEARTBEAT = 0x00;
static constexpr uint8_t GDL90_MSG_OWNSHIP = 0x0a;
static constexpr uint8_t GDL90_MSG_OWNSHIP_GEOM_ALT = 0x0b;
static constexpr uint8_t GDL90_MSG_TRAFFIC = 0x14;
static constexpr uint8_t GDL90_MSG_FOREFLIGHT = 0x65;

/* Minimum logical frame: message id + 2-byte little-endian CRC (payload may
   be empty). Same lower bound applies to stuffed bytes when no escapes. */
static constexpr std::size_t GDL90_MIN_FRAME_BYTES = 3;

static constexpr uint8_t GDL90_MSG_ID_RESERVED_BIT = 0x80;

/* Heartbeat payload (ICD §3.1): status1, status2, LE sec16, msg counts = 6 */
static constexpr std::size_t GDL90_HEARTBEAT_MIN_PAYLOAD = 6;
static constexpr uint8_t GDL90_HEARTBEAT_UTC_VALID_MASK = 0x01;
static constexpr unsigned GDL90_HEARTBEAT_SECONDS_BIT16_SHIFT = 7;
static constexpr unsigned GDL90_HEARTBEAT_SECONDS_HIGH_BIT_MASK = 0x01u;
static constexpr unsigned GDL90_SECONDS_PER_DAY = 24u * 60u * 60u;

/* Traffic / Ownship report payload (ICD §3.5.1), 27 bytes */
static constexpr std::size_t GDL90_REPORT_PAYLOAD_BYTES = 27;
/* Callsign field starts at byte 18 (ICD §3.5.1.11) */
static constexpr std::size_t GDL90_REPORT_CALLSIGN_OFFSET = 18;
/* Callsign is 8 bytes → minimum payload size 26 (last index 25) */
static constexpr std::size_t GDL90_TRAFFIC_MIN_PAYLOAD_FOR_CALLSIGN = 26;

/* Pressure altitude in report (ICD §3.5.1): 25 ft steps, −1000 ft offset */
static constexpr int GDL90_PRESSURE_ALT_FT_LSB = 25;
static constexpr int GDL90_PRESSURE_ALT_FT_OFFSET = 1000;
static constexpr uint16_t GDL90_PRESSURE_ALT_INVALID = 0xfff;
static constexpr unsigned GDL90_ALT_WORD_CODE_SHIFT = 4;
static constexpr uint16_t GDL90_ALT_WORD_MISC_MASK = 0x0f;

/* Ownship Geometric Altitude (ICD §3.6): int16, 5 ft per LSB */
static constexpr int GDL90_GEOM_ALT_FT_RESOLUTION = 5;
static constexpr std::size_t GDL90_GEOM_ALT_PAYLOAD_BYTES = 4;

static constexpr double GDL90_LAT_MIN_DEG = -90;
static constexpr double GDL90_LAT_MAX_DEG = 90;
static constexpr double GDL90_LON_MIN_DEG = -180;
static constexpr double GDL90_LON_MAX_DEG = 180;

/* 24-bit semicircle fraction (ICD §3.5.1.3) */
static constexpr unsigned GDL90_SEMICIRCLE_FRACTION_BITS = 23;

static constexpr uint32_t GDL90_SIGN_EXT24_VALUE_MASK = 0x00ffffff;
static constexpr uint32_t GDL90_SIGN_EXT24_SIGN_BIT = 0x00800000;
static constexpr uint32_t GDL90_SIGN_EXT24_OR_MASK = 0xff000000;

/* Velocity triplet (BE24): upper 12 bits = knots, lower 12 = vert rate. */
static constexpr unsigned GDL90_VEL_HORIZONTAL_SHIFT = 12;
static constexpr uint16_t GDL90_VEL_HORIZONTAL_MASK = 0x0fff;
static constexpr uint16_t GDL90_VEL_HORIZONTAL_INVALID = 0x0fff;
static constexpr uint16_t GDL90_VEL_VERTICAL_MASK = 0x0fff;
/* Vertical field 0x800 = no vertical rate information (ICD §3.5.1) */
static constexpr uint16_t GDL90_VEL_VERTICAL_NO_INFO = 0x0800;
static constexpr uint16_t GDL90_VEL12_SIGN_BIT = 0x0800;
static constexpr int GDL90_VEL12_SIGN_EXTEND = 0x1000;
static constexpr int GDL90_VERTICAL_RATE_FPM_PER_LSB = 64;
static constexpr double GDL90_FPM_TO_METERS_PER_SECOND = 0.00508;

/* Track: byte scale 256/360° (ICD §3.5.1); status in altitude word low bits */
static constexpr uint8_t GDL90_TRACK_BYTE_INVALID = 0xff;
static constexpr uint8_t GDL90_TRACK_STATUS_MASK = 0x03;
static constexpr double GDL90_TRACK_DEG_PER_BYTE = 360.0 / 256.0;

/* Callsign field (ICD §3.5.1.11) */
static constexpr std::size_t GDL90_CALLSIGN_CHAR_COUNT = 8;
static constexpr uint8_t GDL90_CALLSIGN_ASCII_MIN = 0x20;
static constexpr uint8_t GDL90_CALLSIGN_ASCII_MAX = 0x7f;

static constexpr uint32_t GDL90_ICAO_ADDRESS_MASK = 0xffffff;

/* Default traffic filter limits if profile missing (metres) */
static constexpr uint16_t GDL90_DEFAULT_FILTER_HRANGE_M = 20000;
static constexpr uint16_t GDL90_DEFAULT_FILTER_VRANGE_M = 2000;

/* Participant address type high nibble (ICD §3.5.1.2) */
static constexpr uint8_t GDL90_ADDR_ADSB_ICAO = 0;
static constexpr uint8_t GDL90_ADDR_ADSB_SELF_ASSIGNED = 1;
static constexpr uint8_t GDL90_ADDR_TISB_ICAO = 2;
static constexpr uint8_t GDL90_ADDR_TISB_TRACK_FILE = 3;
static constexpr uint8_t GDL90_ADDR_TISB_SURFACE_VEHICLE = 4;
static constexpr uint8_t GDL90_ADDR_ADSR_ICAO = 6;

/* ForeFlight GDL90 extensions (sub-ids, not in public Garmin ICD) */
static constexpr uint8_t GDL90_FOREFLIGHT_SUB_ID_MSG = 0x00;
static constexpr uint8_t GDL90_FOREFLIGHT_SUB_AHRS = 0x01;
static constexpr uint8_t GDL90_FOREFLIGHT_ID_VERSION = 1;
static constexpr std::size_t GDL90_FOREFLIGHT_ID_MIN_PAYLOAD = 38;
static constexpr std::size_t GDL90_FOREFLIGHT_CAPABILITIES_OFFSET = 34;
static constexpr uint32_t GDL90_FOREFLIGHT_CAP_GEO_ALT_IS_MSL = 0x00000001u;
static constexpr std::size_t GDL90_FOREFLIGHT_AHRS_MIN_PAYLOAD = 11;
static constexpr uint16_t GDL90_FOREFLIGHT_ANGLE_INVALID = 0x7fff;
static constexpr uint16_t GDL90_FOREFLIGHT_HEADING_INVALID = 0xffff;
static constexpr uint16_t GDL90_FOREFLIGHT_HEADING_VALUE_MASK = 0x7fff;
static constexpr uint16_t GDL90_FOREFLIGHT_AIRSPEED_INVALID = 0xffff;

#ifdef RASPBERRY_PI
static constexpr bool GDL90_DEFAULT_USE_SYSTEM_UTC_DATE = false;
#else
static constexpr bool GDL90_DEFAULT_USE_SYSTEM_UTC_DATE = true;
#endif

GDL90Device::GDL90Settings gdl90_settings{
  GDL90_DEFAULT_FILTER_HRANGE_M,
  GDL90_DEFAULT_FILTER_VRANGE_M,
  GDL90_DEFAULT_USE_SYSTEM_UTC_DATE,
};

void
LoadFromProfile(GDL90Device::GDL90Settings &settings) noexcept
{
  settings.hrange = GDL90_DEFAULT_FILTER_HRANGE_M;
  settings.vrange = GDL90_DEFAULT_FILTER_VRANGE_M;
  settings.use_system_utc_date = GDL90_DEFAULT_USE_SYSTEM_UTC_DATE;

  Profile::Get(ProfileKeys::GDL90HorizontalRange, settings.hrange);
  Profile::Get(ProfileKeys::GDL90VerticalRange, settings.vrange);
  Profile::Get(ProfileKeys::GDL90UseSystemUtcDate, settings.use_system_utc_date);
}

void
SaveToProfile(GDL90Device::GDL90Settings &settings) noexcept
{
  Profile::Set(ProfileKeys::GDL90HorizontalRange, settings.hrange);
  Profile::Set(ProfileKeys::GDL90VerticalRange, settings.vrange);
  Profile::Set(ProfileKeys::GDL90UseSystemUtcDate, settings.use_system_utc_date);
}

/**
 * Safe byte read for constexpr parsers: out-of-range index yields 0 so
 * ReadBE16/24 never read past the span end when the caller already checked
 * minimum length.
 */
static constexpr uint8_t
ReadU8(std::span<const uint8_t> s, std::size_t i) noexcept
{
  return i < s.size() ? s[i] : 0;
}

/** Big-endian uint16 starting at `i` (high byte first on the wire). */
static constexpr uint16_t
ReadBE16(std::span<const uint8_t> s, std::size_t i) noexcept
{
  return (uint16_t(ReadU8(s, i)) << 8) | uint16_t(ReadU8(s, i + 1));
}

/** Big-endian 24-bit value starting at `i` (ICD multi-byte field order). */
static constexpr uint32_t
ReadBE24(std::span<const uint8_t> s, std::size_t i) noexcept
{
  return (uint32_t(ReadU8(s, i)) << 16) |
    (uint32_t(ReadU8(s, i + 1)) << 8) |
    uint32_t(ReadU8(s, i + 2));
}

/**
 * Interpret `value` as a signed 24-bit integer (latitude/longitude raw).
 * Bit 23 is the sign; upper 8 bits of the 32-bit word are discarded then
 * replicated if negative (two's complement).
 */
static constexpr int32_t
SignExtend24(uint32_t value) noexcept
{
  value &= GDL90_SIGN_EXT24_VALUE_MASK;
  if ((value & GDL90_SIGN_EXT24_SIGN_BIT) != 0)
    return int32_t(value | GDL90_SIGN_EXT24_OR_MASK);
  return int32_t(value);
}

/**
 * GDL90 §3.5.1.3: position is a signed fraction of a semicircle
 * (−2^23 … 2^23−1 maps to −180° … +180°).
 */
static constexpr double
SemiCircles24ToDegrees(int32_t value) noexcept
{
  return double(value) *
    (180.0 / double(1u << GDL90_SEMICIRCLE_FRACTION_BITS));
}

/** CRC-CCITT over `data` using the ICD §2.2.3 table-driven update. */
static uint16_t
Gdl90Crc16(std::span<const uint8_t> data) noexcept
{
  uint16_t crc = 0;

  for (uint8_t b : data)
    crc = crc16ccitt_table[crc >> 8] ^ (crc << 8) ^ b;

  return crc;
}

/**
 * Invert GDL90 byte stuffing: 0x7D means “next byte XOR 0x20”.
 * Malformed input (escape as last byte) returns false.
 */
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

/** Reject ids with MSB set (reserved in ICD §2.2.2). */
static constexpr bool
IsGdl90MessageId(uint8_t id) noexcept
{
  return (id & GDL90_MSG_ID_RESERVED_BIT) == 0;
}

/** Six-digit hex ICAO address for FLARM traffic id matching. */
static FlarmId
MakeIcaoFlarmId(uint32_t icao) noexcept
{
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%06X",
           (unsigned)(icao & GDL90_ICAO_ADDRESS_MASK));
  return FlarmId::Parse(buffer, nullptr);
}

/**
 * Copy 8-byte ICD callsign field: printable ASCII 0x20–0x7E, else space;
 * trim trailing spaces for display.
 */
static void
ExtractCallsign(std::span<const uint8_t, GDL90_CALLSIGN_CHAR_COUNT> src,
                StaticString<10> &dest) noexcept
{
  char tmp[GDL90_CALLSIGN_CHAR_COUNT + 1];
  for (std::size_t i = 0; i < GDL90_CALLSIGN_CHAR_COUNT; ++i) {
    const uint8_t ch = src[i];
    tmp[i] = ch >= GDL90_CALLSIGN_ASCII_MIN && ch < GDL90_CALLSIGN_ASCII_MAX
      ? (char)ch
      : ' ';
  }
  tmp[GDL90_CALLSIGN_CHAR_COUNT] = 0;

  /* trim trailing spaces */
  unsigned n = GDL90_CALLSIGN_CHAR_COUNT;
  while (n > 0 && tmp[n - 1] == ' ')
    --n;
  tmp[n] = 0;

  dest.SetASCII(tmp);
  dest.CleanASCII();
}

static constexpr FlarmTraffic::AircraftType
ToFlarmAircraftType(uint8_t emitter_category) noexcept
{
  /* GDL90 ICD §3.5.1.10 "Emitter Category".  We map a subset to the
     existing FLARM aircraft type enum to populate UI fields. */
  switch (emitter_category) {
  case 1: /* Light */
  case 2: /* Small */
    return FlarmTraffic::AircraftType::POWERED_AIRCRAFT;

  case 3: /* Large */
  case 4: /* High vortex */
  case 5: /* Heavy */
  case 6: /* High performance */
    return FlarmTraffic::AircraftType::JET_AIRCRAFT;

  case 7: /* Rotorcraft */
    return FlarmTraffic::AircraftType::HELICOPTER;

  case 9: /* Glider / sailplane */
    return FlarmTraffic::AircraftType::GLIDER;

  case 10: /* Lighter-than-air */
    return FlarmTraffic::AircraftType::BALLOON;

  case 11: /* Parachutist / skydiver */
    return FlarmTraffic::AircraftType::PARACHUTE;

  case 12: /* Ultralight / hang glider / paraglider */
    return FlarmTraffic::AircraftType::HANG_GLIDER;

  case 14: /* UAV */
    return FlarmTraffic::AircraftType::UAV;

  case 15: /* Space / trans-atmospheric */
  default:
    return FlarmTraffic::AircraftType::UNKNOWN;
  }
}

void
GDL90Device::ParseHeartbeat(std::span<const uint8_t> payload,
                            NMEAInfo &info) noexcept
{
  /* ICD §3.1 heartbeat (payload after message id): [0] status1, [1] status2,
     [2:3] UTC seconds since midnight LE (bits 0..15), bit 16 in status2 bit7.
     Stratux sets UTC-valid in status2 bit0 (see gen_gdl90 makeHeartbeat). */
  if (payload.size() < GDL90_HEARTBEAT_MIN_PAYLOAD)
    return;

  const uint8_t status2 = payload[1];
  if ((status2 & GDL90_HEARTBEAT_UTC_VALID_MASK) == 0)
    return;

  /* 17-bit second count: low 16 LE at [2:3], bit 16 in status2 bit7. */
  const unsigned seconds16 = unsigned(payload[2]) | (unsigned(payload[3]) << 8);
  const unsigned seconds =
    (((status2 >> GDL90_HEARTBEAT_SECONDS_BIT16_SHIFT) &
      GDL90_HEARTBEAT_SECONDS_HIGH_BIT_MASK)
     << 16) |
    seconds16;

  if (seconds >= GDL90_SECONDS_PER_DAY)
    return;

  info.ProvideTime(TimeStamp{FloatDuration{double(seconds)}});

  /* Heartbeat ICD §3.1 carries TOD only; optional host UTC calendar. */
  if (gdl90_settings.use_system_utc_date) {
    const BrokenDateTime now = BrokenDateTime::NowUTC();
    if (now.IsDatePlausible())
      info.ProvideDate(now);
  }
}

void
GDL90Device::ParseOwnshipReport(std::span<const uint8_t> payload,
                                NMEAInfo &info) noexcept
{
  /* Ownship Report (ICD §3.4 / §3.5.1) */
  if (payload.size() < GDL90_REPORT_PAYLOAD_BYTES)
    return;

  const int32_t lat_raw = SignExtend24(ReadBE24(payload, 4));
  const int32_t lon_raw = SignExtend24(ReadBE24(payload, 7));
  const double lat_deg = SemiCircles24ToDegrees(lat_raw);
  const double lon_deg = SemiCircles24ToDegrees(lon_raw);

  if (!std::isfinite(lat_deg) || !std::isfinite(lon_deg) ||
      lat_deg < GDL90_LAT_MIN_DEG || lat_deg > GDL90_LAT_MAX_DEG ||
      lon_deg < GDL90_LON_MIN_DEG || lon_deg > GDL90_LON_MAX_DEG)
    return;

  info.location = GeoPoint(Angle::Degrees(lon_deg), Angle::Degrees(lat_deg));
  info.location_available.Update(info.clock);

  /* Altitude word: bits 15–4 = pressure altitude code; bits 3–0 = misc
     (track-valid flags in low 2 bits per ICD §3.5.1). */
  const uint16_t alt_word = ReadBE16(payload, 10);
  const uint16_t altitude_code = alt_word >> GDL90_ALT_WORD_CODE_SHIFT;
  const uint8_t misc = uint8_t(alt_word & GDL90_ALT_WORD_MISC_MASK);
  if (altitude_code != GDL90_PRESSURE_ALT_INVALID) {
    const int altitude_ft = int(altitude_code) * GDL90_PRESSURE_ALT_FT_LSB -
      GDL90_PRESSURE_ALT_FT_OFFSET;
    const double pressure_altitude =
      Units::ToSysUnit(altitude_ft, Unit::FEET);
    info.ProvidePressureAltitude(pressure_altitude);
  }

  /* 24-bit BE velocity: top 12 bits = ground speed (kt), bottom 12 = vert
     rate; ownship uses horizontal component only here. */
  const uint32_t vel = ReadBE24(payload, 13);
  const uint16_t h_code = uint16_t((vel >> GDL90_VEL_HORIZONTAL_SHIFT) &
                                    GDL90_VEL_HORIZONTAL_MASK);
  if (h_code != GDL90_VEL_HORIZONTAL_INVALID) {
    info.ground_speed = Units::ToSysUnit(h_code, Unit::KNOTS);
    info.ground_speed_available.Update(info.clock);
  }

  const uint8_t track_byte = payload[16];
  const bool track_valid = (misc & GDL90_TRACK_STATUS_MASK) != 0;
  if (track_valid && track_byte != GDL90_TRACK_BYTE_INVALID) {
    info.track =
      Angle::Degrees(double(track_byte) * GDL90_TRACK_DEG_PER_BYTE);
    info.track_available.Update(info.clock);
  }

  /* Basic fix quality (optional but helps status panels) */
  info.gps.fix_quality = FixQuality::GPS;
  info.gps.fix_quality_available.Update(info.clock);
  info.gps.real = true;
}

void
GDL90Device::ParseOwnshipGeometricAltitude(std::span<const uint8_t> payload,
                                           NMEAInfo &info) noexcept
{
  /* Ownship Geometric Altitude (ICD §3.6): int16, 5 ft per LSB */
  if (payload.size() < GDL90_GEOM_ALT_PAYLOAD_BYTES)
    return;

  const int16_t alt5 = int16_t(ReadBE16(payload, 0));
  const double altitude_ft =
    double(alt5) * double(GDL90_GEOM_ALT_FT_RESOLUTION);
  double altitude = Units::ToSysUnit(altitude_ft, Unit::FEET);

  /* ForeFlight's 0x65 ID message can declare that 0x0B is MSL.
     If the sender uses ellipsoid altitude, convert to MSL using EGM96. */
  if (!geo_altitude_is_msl && info.location_available)
    altitude -= EGM96::LookupSeparation(info.location);

  info.gps_altitude = altitude;
  info.gps_altitude_available.Update(info.clock);
}

void
GDL90Device::ParseForeFlight(std::span<const uint8_t> payload,
                             NMEAInfo &info) noexcept
{
  /*
   * ForeFlight 0x65: first payload byte is sub-id (not in Garmin ICD).
   * https://foreflight.com/connect/spec/
   *
   * ID (0x00): version at [1], capabilities BE32 at [34..37]; bit0 selects
   * ellipsoid vs MSL for message 0x0B.
   * AHRS (0x01): BE16 fields at [1],[3],[5],[7],[9] — roll, pitch, heading,
   * IAS, TAS (see Protocol.md).
   */
  if (payload.empty())
    return;

  const uint8_t sub_id = payload[0];

  if (sub_id == GDL90_FOREFLIGHT_SUB_ID_MSG) {
    if (payload.size() < 2)
      return;

    const uint8_t version = payload[1];
    if (version != GDL90_FOREFLIGHT_ID_VERSION)
      return;

    if (payload.size() < GDL90_FOREFLIGHT_ID_MIN_PAYLOAD)
      return;

    const std::size_t cap_o = GDL90_FOREFLIGHT_CAPABILITIES_OFFSET;
    const uint32_t capabilities =
      (uint32_t(payload[cap_o]) << 24) |
      (uint32_t(payload[cap_o + 1]) << 16) |
      (uint32_t(payload[cap_o + 2]) << 8) |
      uint32_t(payload[cap_o + 3]);

    geo_altitude_is_msl =
      (capabilities & GDL90_FOREFLIGHT_CAP_GEO_ALT_IS_MSL) != 0;

    if (info.device.license.empty())
      info.device.license = "ForeFlight";

    return;
  }

  if (sub_id == GDL90_FOREFLIGHT_SUB_AHRS) {
    /* AHRS layout per ForeFlight spec (0.1° except as noted) */
    if (payload.size() < GDL90_FOREFLIGHT_AHRS_MIN_PAYLOAD)
      return;

    const uint16_t roll_u = ReadBE16(payload, 1);
    if (roll_u != GDL90_FOREFLIGHT_ANGLE_INVALID) {
      info.attitude.bank_angle = Angle::Degrees(int16_t(roll_u) / 10.);
      info.attitude.bank_angle_available.Update(info.clock);
    }

    const uint16_t pitch_u = ReadBE16(payload, 3);
    if (pitch_u != GDL90_FOREFLIGHT_ANGLE_INVALID) {
      info.attitude.pitch_angle = Angle::Degrees(int16_t(pitch_u) / 10.);
      info.attitude.pitch_angle_available.Update(info.clock);
    }

    const uint16_t heading_u = ReadBE16(payload, 5);
    if (heading_u != GDL90_FOREFLIGHT_HEADING_INVALID) {
      /* bit15 indicates magnetic vs true heading; XCSoar's attitude heading
         does not track this distinction, store the numeric value. */
      const unsigned heading_tenths =
        unsigned(heading_u & GDL90_FOREFLIGHT_HEADING_VALUE_MASK);
      info.attitude.heading = Angle::Degrees(heading_tenths / 10.);
      info.attitude.heading_available.Update(info.clock);
    }

    const uint16_t ias_u = ReadBE16(payload, 7);
    const uint16_t tas_u = ReadBE16(payload, 9);
    const bool ias_valid = ias_u != GDL90_FOREFLIGHT_AIRSPEED_INVALID;
    const bool tas_valid = tas_u != GDL90_FOREFLIGHT_AIRSPEED_INVALID;

    if (ias_valid && tas_valid) {
      info.ProvideBothAirspeeds(Units::ToSysUnit(ias_u, Unit::KNOTS),
                                Units::ToSysUnit(tas_u, Unit::KNOTS));
    } else if (ias_valid) {
      info.ProvideIndicatedAirspeed(Units::ToSysUnit(ias_u, Unit::KNOTS));
    } else if (tas_valid) {
      info.ProvideTrueAirspeed(Units::ToSysUnit(tas_u, Unit::KNOTS));
    }

    return;
  }
}

void
GDL90Device::ParseTrafficReport(std::span<const uint8_t> payload,
                                NMEAInfo &info) noexcept
{
  /* Traffic Report (ICD §3.5 / §3.5.1) */
  if (payload.size() < GDL90_REPORT_PAYLOAD_BYTES)
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
      lat_deg < GDL90_LAT_MIN_DEG || lat_deg > GDL90_LAT_MAX_DEG ||
      lon_deg < GDL90_LON_MIN_DEG || lon_deg > GDL90_LON_MAX_DEG)
    return;

  /* Same altitude / misc layout as ownship (ICD §3.5.1). */
  const uint16_t alt_word = ReadBE16(payload, 10);
  const uint16_t altitude_code = alt_word >> GDL90_ALT_WORD_CODE_SHIFT;
  const uint8_t misc = uint8_t(alt_word & GDL90_ALT_WORD_MISC_MASK);

  bool altitude_available = altitude_code != GDL90_PRESSURE_ALT_INVALID;
  double altitude = 0;
  if (altitude_available) {
    const int altitude_ft = int(altitude_code) * GDL90_PRESSURE_ALT_FT_LSB -
      GDL90_PRESSURE_ALT_FT_OFFSET;
    altitude = Units::ToSysUnit(altitude_ft, Unit::FEET);
  }

  if (gdl90_settings.hrange > 0 && info.location_available) {
    const GeoPoint target_location(Angle::Degrees(lon_deg),
                                   Angle::Degrees(lat_deg));
    const GeoVector vec{info.location, target_location};
    if (vec.distance > gdl90_settings.hrange)
      return;
  }

  /* Traffic reports carry pressure altitude (ICD §3.5.1). Compare only to
     ownship pressure — not GPS/geometric (0x0B), or a bogus delta can exceed
     vrange and drop every target (e.g. Stratux with geom alt but no baro). */
  if (gdl90_settings.vrange > 0 && altitude_available &&
      info.pressure_altitude_available) {
    if (fabs(altitude - info.pressure_altitude) > gdl90_settings.vrange)
      return;
  }

  const uint8_t nacp_nic = payload[12];
  (void)nacp_nic; /* not used yet */

  const uint32_t vel = ReadBE24(payload, 13);
  const uint16_t h_code = uint16_t((vel >> GDL90_VEL_HORIZONTAL_SHIFT) &
                                    GDL90_VEL_HORIZONTAL_MASK);
  const uint16_t v_code = uint16_t(vel & GDL90_VEL_VERTICAL_MASK);

  bool speed_received = h_code != GDL90_VEL_HORIZONTAL_INVALID;
  double speed = 0;
  if (speed_received)
    speed = Units::ToSysUnit(h_code, Unit::KNOTS);

  /* Lower 12 bits: vertical speed; 0x800 means “no information”. */
  bool climb_rate_received = v_code != GDL90_VEL_VERTICAL_NO_INFO;
  double climb_rate = 0;
  if (climb_rate_received) {
    /* Sign-extend 12-bit two's complement to 16 bits. */
    int16_t v_signed = int16_t(v_code);
    if ((v_signed & int16_t(GDL90_VEL12_SIGN_BIT)) != 0)
      v_signed = int16_t(v_signed - GDL90_VEL12_SIGN_EXTEND);

    climb_rate = double(v_signed) * double(GDL90_VERTICAL_RATE_FPM_PER_LSB) *
      GDL90_FPM_TO_METERS_PER_SECOND;
  }

  const uint8_t track_byte = payload[16];
  const bool track_valid = (misc & GDL90_TRACK_STATUS_MASK) != 0;
  bool track_received = track_valid;
  Angle track = Angle::Zero();
  if (track_received && track_byte != GDL90_TRACK_BYTE_INVALID)
    track = Angle::Degrees(double(track_byte) * GDL90_TRACK_DEG_PER_BYTE);
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

  if (payload.size() >= GDL90_TRAFFIC_MIN_PAYLOAD_FOR_CALLSIGN) {
    const std::span<const uint8_t, GDL90_CALLSIGN_CHAR_COUNT> callsign{
      payload.data() + GDL90_REPORT_CALLSIGN_OFFSET,
      GDL90_CALLSIGN_CHAR_COUNT};
    ExtractCallsign(callsign, slot->name);
  }

  slot->alarm_level = FlarmTraffic::AlarmType::NONE;
  slot->type = ToFlarmAircraftType(payload[17]);

  /* id_type: ICAO for stable 24-bit addresses; RANDOM for self-assigned /
     track-file IDs so FlarmComputer skips callsign DB lookup (see FTD-012). */
  switch (address_type) {
  case GDL90_ADDR_ADSB_ICAO:
    slot->source = FlarmTraffic::SourceType::ADSB;
    slot->id_type = FlarmTraffic::IdType::ICAO;
    break;

  case GDL90_ADDR_ADSB_SELF_ASSIGNED:
    slot->source = FlarmTraffic::SourceType::ADSB;
    slot->id_type = FlarmTraffic::IdType::RANDOM;
    break;

  case GDL90_ADDR_TISB_ICAO:
  case GDL90_ADDR_TISB_SURFACE_VEHICLE:
    slot->source = FlarmTraffic::SourceType::TISB;
    slot->id_type = FlarmTraffic::IdType::ICAO;
    break;

  case GDL90_ADDR_TISB_TRACK_FILE:
    slot->source = FlarmTraffic::SourceType::TISB;
    slot->id_type = FlarmTraffic::IdType::RANDOM;
    break;

  case GDL90_ADDR_ADSR_ICAO:
    slot->source = FlarmTraffic::SourceType::ADSR;
    slot->id_type = FlarmTraffic::IdType::ICAO;
    break;

  default:
    slot->source = FlarmTraffic::SourceType::ADSB;
    slot->id_type = FlarmTraffic::IdType::ICAO;
    break;
  }

  slot->location = GeoPoint(Angle::Degrees(lon_deg), Angle::Degrees(lat_deg));
  slot->location_available = true;
  slot->absolute_location = true;

  if (altitude_available) {
    slot->altitude = altitude;
    slot->altitude_available = true;
    slot->absolute_altitude = true;
  } else {
    slot->altitude = 0;
    slot->altitude_available = false;
    slot->absolute_altitude = false;
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
  /* Accumulate bytes, split on 0x7E, unstuff, CRC-check, then dispatch on
     message id. Payload passed to Parse* is everything after id, before CRC. */
  const auto *p = reinterpret_cast<const uint8_t *>(s.data());
  buffer.insert(buffer.end(), p, p + s.size());

  unescaped.reserve(GDL90_UNESCAPED_INITIAL_CAPACITY);

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
      if (buffer.size() > GDL90_BUFFER_TRIM_SIZE)
        buffer.erase(buffer.begin(),
                     buffer.end() -
                       static_cast<std::ptrdiff_t>(GDL90_BUFFER_TRIM_SIZE));
      break;
    }

    /* Copy stuffed payload before buffer.erase: span into buffer is
       invalidated by erase (reallocation / invalid pointers). */
    escaped_frame.assign(buffer.begin() + 1, end);

    buffer.erase(buffer.begin(), end + 1);

    if (escaped_frame.size() < GDL90_MIN_FRAME_BYTES)
      continue;

    if (escaped_frame.size() > GDL90_MAX_FRAME)
      continue;

    if (!UnescapeGdl90(escaped_frame, unescaped))
      continue;

    if (unescaped.size() < GDL90_MIN_FRAME_BYTES)
      continue;

    const uint8_t msg_id = unescaped[0];
    if (!IsGdl90MessageId(msg_id))
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
    case GDL90_MSG_HEARTBEAT:
      ParseHeartbeat(payload, info);
      break;

    case GDL90_MSG_OWNSHIP:
      ParseOwnshipReport(payload, info);
      break;

    case GDL90_MSG_OWNSHIP_GEOM_ALT:
      ParseOwnshipGeometricAltitude(payload, info);
      break;

    case GDL90_MSG_TRAFFIC:
      ParseTrafficReport(payload, info);
      break;

    case GDL90_MSG_FOREFLIGHT:
      ParseForeFlight(payload, info);
      break;
    }
  }

  return true;
}
