// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/GDL90/Driver.hpp"
#include "FLARM/Computer.hpp"
#include "FLARM/Id.hpp"
#include "Geo/Geoid.hpp"
#include "NMEA/Info.hpp"
#include "TestUtil.hpp"
#include "Units/System.hpp"
#include "util/CRC16CCITT.hpp"

#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>

static constexpr uint8_t GDL90_FLAG = 0x7e;
static constexpr uint8_t GDL90_ESCAPE = 0x7d;
static constexpr uint8_t GDL90_ESCAPE_XOR = 0x20;

static constexpr uint8_t OWNERSHIP_FRAME[] = {
  0x7E, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x15, 0xA7, 0xE5, 0xBA,
  0x47, 0x99, 0x08, 0xC9, 0x88, 0xFF, 0xE0, 0x00, 0x80, 0x01,
  0x4E, 0x31, 0x32, 0x33, 0x34, 0x35, 0x20, 0x20, 0x00, 0x7B,
  0xE5, 0x7E,
};

static NMEAInfo
MakeBlankNMEAInfo() noexcept
{
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};
  return info;
}

static void
FeedOwnship(GDL90Device &dev, NMEAInfo &info) noexcept
{
  dev.DataReceived({reinterpret_cast<const std::byte *>(OWNERSHIP_FRAME),
                    sizeof(OWNERSHIP_FRAME)}, info);
}

static uint16_t
Gdl90Crc16(const uint8_t *data, std::size_t size) noexcept
{
  /* CRC-CCITT update function per GDL90 ICD 2.2.3 (table-driven). */
  uint16_t crc = 0;
  for (std::size_t i = 0; i < size; ++i)
    crc = crc16ccitt_table[crc >> 8] ^ (crc << 8) ^ data[i];

  return crc;
}

static void
AppendEscaped(std::vector<uint8_t> &out, uint8_t b)
{
  if (b == GDL90_FLAG || b == GDL90_ESCAPE) {
    out.push_back(GDL90_ESCAPE);
    out.push_back(b ^ GDL90_ESCAPE_XOR);
  } else
    out.push_back(b);
}

static std::vector<uint8_t>
BuildFrame(uint8_t msg_id, const uint8_t *payload, std::size_t payload_size)
{
  std::vector<uint8_t> body;
  body.reserve(1 + payload_size + 2);

  body.push_back(msg_id);
  body.insert(body.end(), payload, payload + payload_size);

  const uint16_t crc = Gdl90Crc16(body.data(), body.size());
  body.push_back(uint8_t(crc & 0xff));
  body.push_back(uint8_t(crc >> 8));

  std::vector<uint8_t> framed;
  framed.reserve(2 + body.size() * 2);
  framed.push_back(GDL90_FLAG);
  for (uint8_t b : body)
    AppendEscaped(framed, b);
  framed.push_back(GDL90_FLAG);
  return framed;
}

static int32_t
DegreesToSemiCircles24(double deg) noexcept
{
  long value = lround(deg * (double(1u << 23) / 180.0));
  if (value < -(1l << 23))
    value = -(1l << 23);
  if (value > (1l << 23) - 1)
    value = (1l << 23) - 1;
  return int32_t(value);
}

static void
WriteBE24(uint8_t *dest, int32_t value) noexcept
{
  const uint32_t u = uint32_t(value) & 0x00ffffff;
  dest[0] = uint8_t(u >> 16);
  dest[1] = uint8_t(u >> 8);
  dest[2] = uint8_t(u);
}

static std::vector<uint8_t>
BuildTrafficFrame(uint32_t icao, double lat_deg, double lon_deg,
                  bool altitude_available, unsigned altitude_ft) noexcept
{
  uint8_t payload[27]{};
  payload[0] = 0x00; /* address_type=0, alert=0 */
  payload[1] = uint8_t((icao >> 16) & 0xff);
  payload[2] = uint8_t((icao >> 8) & 0xff);
  payload[3] = uint8_t(icao & 0xff);

  WriteBE24(payload + 4, DegreesToSemiCircles24(lat_deg));
  WriteBE24(payload + 7, DegreesToSemiCircles24(lon_deg));

  const uint8_t misc = 0x01;
  const uint16_t alt_code = altitude_available
    ? uint16_t((altitude_ft + 1000) / 25)
    : 0x0fff;
  const uint16_t alt_word = uint16_t((alt_code << 4) | misc);
  payload[10] = uint8_t(alt_word >> 8);
  payload[11] = uint8_t(alt_word & 0xff);

  payload[16] = 0xff; /* track N/A */
  payload[17] = 0x01; /* emitter category */
  payload[26] = 0x00;

  return BuildFrame(0x14, payload, sizeof(payload));
}

static std::vector<uint8_t>
BuildForeFlightIdFrame(bool geo_altitude_is_msl) noexcept
{
  /* ForeFlight 0x65 ID message (sub-id 0, version 1):
     https://foreflight.com/connect/spec/ */
  uint8_t payload[38]{};
  payload[0] = 0x00; /* sub-id */
  payload[1] = 0x01; /* version */

  /* capabilities mask (big-endian) at payload[34..37], bit0 = datum */
  payload[37] = geo_altitude_is_msl ? 0x01 : 0x00;

  return BuildFrame(0x65, payload, sizeof(payload));
}

static std::vector<uint8_t>
BuildForeFlightAhrsFrame(int roll_tenths_deg, int pitch_tenths_deg,
                         unsigned heading_tenths_deg,
                         bool heading_magnetic,
                         unsigned ias_kt,
                         unsigned tas_kt) noexcept
{
  uint8_t payload[11]{};
  payload[0] = 0x01; /* sub-id (AHRS) */

  payload[1] = uint8_t((roll_tenths_deg >> 8) & 0xff);
  payload[2] = uint8_t(roll_tenths_deg & 0xff);

  payload[3] = uint8_t((pitch_tenths_deg >> 8) & 0xff);
  payload[4] = uint8_t(pitch_tenths_deg & 0xff);

  uint16_t heading = uint16_t(heading_tenths_deg & 0x7fff);
  if (heading_magnetic)
    heading |= 0x8000;
  payload[5] = uint8_t(heading >> 8);
  payload[6] = uint8_t(heading & 0xff);

  payload[7] = uint8_t((ias_kt >> 8) & 0xff);
  payload[8] = uint8_t(ias_kt & 0xff);

  payload[9] = uint8_t((tas_kt >> 8) & 0xff);
  payload[10] = uint8_t(tas_kt & 0xff);

  return BuildFrame(0x65, payload, sizeof(payload));
}

static std::vector<uint8_t>
BuildFrameBadCrc(uint8_t msg_id, const uint8_t *payload,
                 std::size_t payload_size)
{
  auto frame = BuildFrame(msg_id, payload, payload_size);

  /* flip one byte inside the frame body to make CRC invalid */
  if (frame.size() >= 4)
    frame[2] ^= 0x01;

  return frame;
}

static void
Feed(GDL90Device &dev, NMEAInfo &info, const std::vector<uint8_t> &bytes)
{
  const auto *p = reinterpret_cast<const std::byte *>(bytes.data());
  dev.DataReceived({p, bytes.size()}, info);
}

static void
TestBadCrcIgnored() noexcept
{
  static constexpr uint8_t payload[] = {
    0x00,
    0x01, 0x02, 0x03,
    0x20, 0x00, 0x00,
    0x20, 0x00, 0x00,
    0x03, 0xC0,
    0x00,
    0x00, 0x10, 0x00,
    0x00,
    0x01,
    'B','A','D','C','R','C',' ',' ',
    0x00,
  };

  const auto frame = BuildFrameBadCrc(0x14, payload, sizeof(payload));

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  Feed(dev, info, frame);

  ok1(!info.alive);
  ok1(info.flarm.traffic.IsEmpty());
}

static void
TestHeartbeatTimeOfDay() noexcept
{
  /* Heartbeat payload with UTC time-of-day. */
  static constexpr unsigned seconds = 12345;
  static_assert(seconds < 24u * 60u * 60u);

  uint8_t payload[6]{};
  payload[0] = 0x01; /* status1 */
  payload[1] = uint8_t(((seconds >> 16) & 0x01) << 7) | 0x01; /* status2 */
  payload[2] = uint8_t(seconds & 0xff);
  payload[3] = uint8_t((seconds >> 8) & 0xff);

  const auto frame = BuildFrame(0x00, payload, sizeof(payload));

  const bool saved_use_system_utc_date = gdl90_settings.use_system_utc_date;
  gdl90_settings.use_system_utc_date = true;

  GDL90Device dev;
  NMEAInfo info = MakeBlankNMEAInfo();

  Feed(dev, info, frame);

  gdl90_settings.use_system_utc_date = saved_use_system_utc_date;

  ok1(info.time_available);
  ok1(fabs(info.time.ToDuration().count() - double(seconds)) < 0.01);
  ok1(info.date_time_utc.IsDatePlausible());
}

static void
TestResyncAfterBadCrc() noexcept
{
  static constexpr uint8_t payload_bad[] = {
    0x00,
    0x01, 0x02, 0x03,
    0x20, 0x00, 0x00,
    0x20, 0x00, 0x00,
    0x03, 0xC0,
    0x00,
    0x00, 0x10, 0x00,
    0x00,
    0x01,
    'B','A','D',' ',' ',' ',' ',' ',
    0x00,
  };

  static constexpr uint8_t payload_good[] = {
    0x00,
    0x0A, 0x0B, 0x0C,
    0x20, 0x00, 0x00,
    0x20, 0x00, 0x00,
    0x03, 0xC0,
    0x00,
    0x00, 0x10, 0x00,
    0x00,
    0x01,
    'G','O','O','D',' ',' ',' ',' ',
    0x00,
  };

  auto bytes = BuildFrameBadCrc(0x14, payload_bad, sizeof(payload_bad));
  const auto good = BuildFrame(0x14, payload_good, sizeof(payload_good));
  bytes.insert(bytes.end(), good.begin(), good.end());

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  Feed(dev, info, bytes);

  ok1(info.alive);
  ok1(!info.flarm.traffic.IsEmpty());
}

static void
TestNoisePrefix() noexcept
{
  static constexpr uint8_t payload[] = {
    0x00,
    0x11, 0x22, 0x33,
    0x20, 0x00, 0x00,
    0x20, 0x00, 0x00,
    0x03, 0xC0,
    0x00,
    0x00, 0x10, 0x00,
    0x00,
    0x01,
    'N','O','I','S','E',' ',' ',' ',
    0x00,
  };

  auto bytes = BuildFrame(0x14, payload, sizeof(payload));
  bytes.insert(bytes.begin(), {0x00, 0x01, 0x02, 0x7d, 0x20, 0x55});

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  Feed(dev, info, bytes);

  ok1(info.alive);
  ok1(!info.flarm.traffic.IsEmpty());
}

static void
TestTwoFramesOneChunk() noexcept
{
  static constexpr uint8_t payload_a[] = {
    0x00,
    0x01, 0x02, 0x03,
    0x20, 0x00, 0x00,
    0x20, 0x00, 0x00,
    0x03, 0xC0,
    0x00,
    0x00, 0x10, 0x00,
    0x00,
    0x01,
    'A',' ',' ',' ',' ',' ',' ',' ',
    0x00,
  };

  static constexpr uint8_t payload_b[] = {
    0x00,
    0x04, 0x05, 0x06,
    0x20, 0x00, 0x00,
    0x20, 0x00, 0x00,
    0x03, 0xC0,
    0x00,
    0x00, 0x10, 0x00,
    0x00,
    0x01,
    'B',' ',' ',' ',' ',' ',' ',' ',
    0x00,
  };

  auto bytes = BuildFrame(0x14, payload_a, sizeof(payload_a));
  const auto b = BuildFrame(0x14, payload_b, sizeof(payload_b));
  bytes.insert(bytes.end(), b.begin(), b.end());

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  Feed(dev, info, bytes);

  ok1(info.alive);
  ok1(!info.flarm.traffic.IsEmpty());
}

static void
TestEscapingInPayload() noexcept
{
  /* Force escaping via participant_address bytes 0x7e and 0x7d. */
  static constexpr uint8_t payload[] = {
    0x00,
    0x7E, 0x7D, 0x01,
    0x20, 0x00, 0x00,
    0x20, 0x00, 0x00,
    0x03, 0xC0,
    0x00,
    0x00, 0x10, 0x00,
    0x00,
    0x01,
    'E','S','C',' ',' ',' ',' ',' ',
    0x00,
  };

  const auto frame = BuildFrame(0x14, payload, sizeof(payload));

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  Feed(dev, info, frame);

  ok1(info.alive);
  ok1(!info.flarm.traffic.IsEmpty());
  const FlarmId expected = FlarmId::Parse("7E7D01", nullptr);
  ok1(info.flarm.traffic.FindTraffic(expected) != nullptr);
}

static void
TestOwnshipDriver() noexcept
{
  GDL90Device dev;
  NMEAInfo info = MakeBlankNMEAInfo();

  FeedOwnship(dev, info);

  ok1(info.alive);
  ok1(info.location_available);
  ok1(between(info.location.latitude.Degrees(), 30.0, 31.0));
  ok1(between(info.location.longitude.Degrees(), -99.0, -98.0));
  ok1(info.pressure_altitude_available);
  ok1(between(info.pressure_altitude,
              Units::ToSysUnit(2495, Unit::FEET),
              Units::ToSysUnit(2505, Unit::FEET)));
}

static void
TestOwnshipGeometricAltitude() noexcept
{
  static constexpr uint8_t payload[] = {
    0x03, 0xE8, /* 1000 * 5ft = 5000ft */
    0x00, 0x00, /* FOM (ignored) */
  };

  const auto frame = BuildFrame(0x0b, payload, sizeof(payload));

  GDL90Device dev;
  NMEAInfo info = MakeBlankNMEAInfo();

  Feed(dev, info, frame);

  ok1(info.gps_altitude_available);
  ok1(between(info.gps_altitude,
              Units::ToSysUnit(4990, Unit::FEET),
              Units::ToSysUnit(5010, Unit::FEET)));
}

static void
TestOwnshipGeometricAltitudeDatum() noexcept
{
  static constexpr uint8_t payload[] = {
    0x03, 0xE8, /* 1000 * 5ft = 5000ft */
    0x00, 0x00,
  };

  const auto id_ellipsoid = BuildForeFlightIdFrame(false);
  const auto id_msl = BuildForeFlightIdFrame(true);
  const auto geo_alt = BuildFrame(0x0b, payload, sizeof(payload));

  {
    GDL90Device dev;
    NMEAInfo info = MakeBlankNMEAInfo();

    info.location = GeoPoint(Angle::Degrees(11), Angle::Degrees(48));
    info.location_available.Update(info.clock);

    Feed(dev, info, id_ellipsoid);
    Feed(dev, info, geo_alt);

    const double expected = Units::ToSysUnit(5000, Unit::FEET) -
      EGM96::LookupSeparation(info.location);

    ok1(info.gps_altitude_available);
    ok1(between(info.gps_altitude, expected - 1, expected + 1));
  }

  {
    GDL90Device dev;
    NMEAInfo info = MakeBlankNMEAInfo();

    info.location = GeoPoint(Angle::Degrees(11), Angle::Degrees(48));
    info.location_available.Update(info.clock);

    Feed(dev, info, id_msl);
    Feed(dev, info, geo_alt);

    const double expected = Units::ToSysUnit(5000, Unit::FEET);
    ok1(info.gps_altitude_available);
    ok1(between(info.gps_altitude, expected - 1, expected + 1));
  }
}

static void
TestForeFlightAhrs() noexcept
{
  const auto frame = BuildForeFlightAhrsFrame(123, -45, 2000, false, 70, 75);

  GDL90Device dev;
  NMEAInfo info = MakeBlankNMEAInfo();

  Feed(dev, info, frame);

  ok1(info.attitude.bank_angle_available);
  ok1(between(info.attitude.bank_angle.Degrees(), 12.2, 12.4));

  ok1(info.attitude.pitch_angle_available);
  ok1(between(info.attitude.pitch_angle.Degrees(), -4.6, -4.4));

  ok1(info.attitude.heading_available);
  ok1(between(info.attitude.heading.Degrees(), 199.9, 200.1));

  ok1(info.airspeed_available);
  ok1(between(info.indicated_airspeed,
              Units::ToSysUnit(69.5, Unit::KNOTS),
              Units::ToSysUnit(70.5, Unit::KNOTS)));
  ok1(between(info.true_airspeed,
              Units::ToSysUnit(74.5, Unit::KNOTS),
              Units::ToSysUnit(75.5, Unit::KNOTS)));
}

static void
TestTrafficDriver() noexcept
{
  /* Traffic Report payload (27 bytes) with ICAO = 0xAB4549.
     The bytes are from the GDL90 report layout examples. */
  static constexpr uint8_t payload[] = {
    0x00,             // status/address type nibble
    0xAB, 0x45, 0x49, // participant address (big-endian 24-bit)
    0x1F, 0xEF, 0x15, // latitude
    0xA8, 0x89, 0x78, // longitude
    0x0F, 0x09,       // altitude/misc (big-endian)
    0xA9,             // NACp/NIC
    0x07, 0xB0, 0x01, // velocity
    0x20,             // track
    0x01,             // emitter category
    0x4E, 0x38, 0x32, 0x35, 0x56, 0x20, 0x20, 0x20, // callsign
    0x00,             // emergency/reserved
  };

  const auto frame = BuildFrame(0x14, payload, sizeof(payload));

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  Feed(dev, info, frame);

  ok1(info.alive);
  ok1(!info.flarm.traffic.IsEmpty());

  const FlarmId expected = FlarmId::Parse("AB4549", nullptr);
  const FlarmTraffic *t = info.flarm.traffic.FindTraffic(expected);
  ok1(t != nullptr);
  ok1(t != nullptr && t->location_available);
  ok1(t != nullptr && t->name.equals("N825V"));
  ok1(t != nullptr && t->type == FlarmTraffic::AircraftType::POWERED_AIRCRAFT);
  ok1(t != nullptr && t->source == FlarmTraffic::SourceType::ADSB);
  ok1(t != nullptr && t->id_type == FlarmTraffic::IdType::ICAO);
  ok1(t != nullptr && t->altitude_available);
  ok1(t != nullptr && between(t->altitude,
                              Units::ToSysUnit(4990, Unit::FEET),
                              Units::ToSysUnit(5010, Unit::FEET)));
}

static void
TestTrafficAddressTypeSource() noexcept
{
  /* Same geometry as TestTrafficDriver; only address-type nibble changes. */
  static constexpr uint8_t base_payload[] = {
    0x00,             // replaced per case
    0xAB, 0x45, 0x49,
    0x1F, 0xEF, 0x15,
    0xA8, 0x89, 0x78,
    0x0F, 0x09,
    0xA9,
    0x07, 0xB0, 0x01,
    0x20,
    0x01,
    0x4E, 0x38, 0x32, 0x35, 0x56, 0x20, 0x20, 0x20,
    0x00,
  };

  struct Case {
    uint8_t status_byte;
    FlarmTraffic::SourceType expect_source;
    FlarmTraffic::IdType expect_id_type;
  };

  static constexpr Case cases[] = {
    {0x10, FlarmTraffic::SourceType::ADSB, FlarmTraffic::IdType::RANDOM},
    {0x20, FlarmTraffic::SourceType::TISB, FlarmTraffic::IdType::ICAO},
    {0x30, FlarmTraffic::SourceType::TISB, FlarmTraffic::IdType::RANDOM},
    {0x40, FlarmTraffic::SourceType::TISB, FlarmTraffic::IdType::ICAO},
    {0x60, FlarmTraffic::SourceType::ADSR, FlarmTraffic::IdType::ICAO},
  };

  for (const Case &c : cases) {
    uint8_t payload[sizeof(base_payload)];
    memcpy(payload, base_payload, sizeof(payload));
    payload[0] = c.status_byte;

    const auto frame = BuildFrame(0x14, payload, sizeof(payload));

    GDL90Device dev;
    NMEAInfo info;
    info.Reset();
    info.clock = TimeStamp{FloatDuration{1}};

    Feed(dev, info, frame);

    const FlarmId expected = FlarmId::Parse("AB4549", nullptr);
    const FlarmTraffic *t = info.flarm.traffic.FindTraffic(expected);
    ok1(t != nullptr);
    ok1(t != nullptr && t->source == c.expect_source);
    ok1(t != nullptr && t->id_type == c.expect_id_type);
  }
}

static void
TestRelativeAltitudeFromPressureAltitude() noexcept
{
  static constexpr uint8_t traffic_payload[] = {
    0x00,             // status/address type nibble
    0xAB, 0x45, 0x49, // participant address (big-endian 24-bit)
    0x1F, 0xEF, 0x15, // latitude
    0xA8, 0x89, 0x78, // longitude
    0x0F, 0x09,       // altitude/misc (big-endian): 5000ft
    0xA9,             // NACp/NIC
    0x07, 0xB0, 0x01, // velocity
    0x20,             // track
    0x01,             // emitter category
    0x4E, 0x38, 0x32, 0x35, 0x56, 0x20, 0x20, 0x20, // callsign
    0x00,             // emergency/reserved
  };

  const auto traffic_frame = BuildFrame(0x14, traffic_payload,
                                        sizeof(traffic_payload));

  const uint16_t saved_hrange = gdl90_settings.hrange;
  const uint16_t saved_vrange = gdl90_settings.vrange;
  gdl90_settings.hrange = 0;
  gdl90_settings.vrange = 0;

  GDL90Device dev;
  NMEAInfo info = MakeBlankNMEAInfo();

  FeedOwnship(dev, info);
  Feed(dev, info, traffic_frame);

  gdl90_settings.hrange = saved_hrange;
  gdl90_settings.vrange = saved_vrange;

  ok1(info.pressure_altitude_available);

  const FlarmId expected = FlarmId::Parse("AB4549", nullptr);
  const FlarmTraffic *t = info.flarm.traffic.FindTraffic(expected);
  ok1(t != nullptr);
  ok1(t != nullptr && t->absolute_altitude);
  ok1(t != nullptr && t->altitude_available);

  FlarmComputer computer;
  FlarmData last_flarm{};
  computer.Process(info.flarm, last_flarm, info);

  /* Ownship (2500ft) vs traffic (5000ft) => +2500ft (~762m). */
  const FlarmTraffic *t2 = info.flarm.traffic.FindTraffic(expected);
  ok1(t2 != nullptr &&
      between(t2->relative_altitude, 760.0, 764.0));
}

static void
TestHorizontalRangeFilter() noexcept
{
  GDL90Device dev;
  NMEAInfo info = MakeBlankNMEAInfo();

  FeedOwnship(dev, info);
  ok1(info.location_available);

  gdl90_settings.hrange = 100; /* 100m */
  gdl90_settings.vrange = 0;

  const auto far = BuildTrafficFrame(0x010203,
                                     info.location.latitude.Degrees() + 0.02,
                                     info.location.longitude.Degrees(),
                                     false, 0);
  Feed(dev, info, far);
  ok1(info.flarm.traffic.IsEmpty());

  const auto near = BuildTrafficFrame(0x010204,
                                      info.location.latitude.Degrees(),
                                      info.location.longitude.Degrees(),
                                      false, 0);
  Feed(dev, info, near);
  ok1(!info.flarm.traffic.IsEmpty());

  gdl90_settings.hrange = 20000;
  gdl90_settings.vrange = 2000;
}

static void
TestVerticalRangeFilter() noexcept
{
  GDL90Device dev;
  NMEAInfo info = MakeBlankNMEAInfo();

  FeedOwnship(dev, info);
  ok1(info.pressure_altitude_available);

  gdl90_settings.hrange = 0;
  gdl90_settings.vrange = 100; /* 100m */

  const auto high = BuildTrafficFrame(0x010205,
                                      info.location.latitude.Degrees(),
                                      info.location.longitude.Degrees(),
                                      true, 6000);
  Feed(dev, info, high);
  ok1(info.flarm.traffic.IsEmpty());

  const auto close = BuildTrafficFrame(0x010206,
                                       info.location.latitude.Degrees(),
                                       info.location.longitude.Degrees(),
                                       true, 2800);
  Feed(dev, info, close);
  ok1(!info.flarm.traffic.IsEmpty());

  gdl90_settings.hrange = 20000;
  gdl90_settings.vrange = 2000;
}

static void
TestVerticalFilterSkippedWithoutOwnshipPressure() noexcept
{
  /* Traffic altitude is pressure; vertical filter must not use GPS MSL as
     reference when baro is missing (else vrange rejects all targets). */
  const uint16_t saved_h = gdl90_settings.hrange;
  const uint16_t saved_v = gdl90_settings.vrange;
  gdl90_settings.hrange = 0;
  gdl90_settings.vrange = 100;

  GDL90Device dev;
  NMEAInfo info = MakeBlankNMEAInfo();
  FeedOwnship(dev, info);
  ok1(info.pressure_altitude_available);
  info.pressure_altitude_available.Clear();
  info.gps_altitude = Units::ToSysUnit(12000, Unit::FEET);
  info.gps_altitude_available.Update(info.clock);

  const auto frame = BuildTrafficFrame(0x010208,
                                       info.location.latitude.Degrees(),
                                       info.location.longitude.Degrees(),
                                       true, 2800);
  Feed(dev, info, frame);
  ok1(!info.flarm.traffic.IsEmpty());

  gdl90_settings.hrange = saved_h;
  gdl90_settings.vrange = saved_v;
}

static void
TestChunkedOwnshipFrame() noexcept
{
  /* Feed one framed Ownship Report in small chunks to validate buffering. */
  GDL90Device dev;
  NMEAInfo info = MakeBlankNMEAInfo();

  const auto *bytes = reinterpret_cast<const std::byte *>(OWNERSHIP_FRAME);
  dev.DataReceived({bytes, 3}, info);
  ok1(!info.location_available);

  dev.DataReceived({bytes + 3, 5}, info);
  ok1(!info.location_available);

  dev.DataReceived({bytes + 8, sizeof(OWNERSHIP_FRAME) - 8}, info);
  ok1(info.location_available);
  ok1(between(info.location.latitude.Degrees(), 30.0, 31.0));
}

static void
TestNegativeCoordinates() noexcept
{
  /* Validate sign-extension (ICD §3.5.1.3) using -45° lat/lon (0xE0 00 00). */
  static constexpr uint8_t payload[] = {
    0x00,             // status/address type nibble
    0x01, 0x02, 0x03, // participant address (big-endian 24-bit)
    0xE0, 0x00, 0x00, // latitude  -45.0
    0xE0, 0x00, 0x00, // longitude -45.0
    0x03, 0xC0,       // altitude/misc (big-endian): 500ft => (500+1000)/25=60 => 0x03C, +m=0
    0x00,             // NACp/NIC
    0x00, 0x10, 0x00, // velocity (arbitrary, valid)
    0x00,             // track
    0x01,             // emitter category
    'T','E','S','T',' ',' ',' ',' ', // callsign
    0x00,             // emergency/reserved
  };

  const auto frame = BuildFrame(0x14, payload, sizeof(payload));

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  Feed(dev, info, frame);
  ok1(!info.flarm.traffic.IsEmpty());

  const FlarmId expected = FlarmId::Parse("010203", nullptr);
  const FlarmTraffic *t = info.flarm.traffic.FindTraffic(expected);
  ok1(t != nullptr);

  ok1(between(t->location.latitude.Degrees(), -46.0, -44.0));
  ok1(between(t->location.longitude.Degrees(), -46.0, -44.0));
}

static void
TestUnavailableFields() noexcept
{
  /* Exercise special values: altitude=0xFFF (invalid), hvel=0xFFF (N/A),
     vvel=0x800 (N/A), track=0xFF (N/A). */
  static constexpr uint8_t payload[] = {
    0x00,
    0x0A, 0x0B, 0x0C,
    0x20, 0x00, 0x00, // lat +45
    0x20, 0x00, 0x00, // lon +45
    0xFF, 0xF0,       // altitude_code=0xFFF, m=0
    0x00,
    0xFF, 0xF8, 0x00, // h=0xFFF, v=0x800
    0xFF,             // track unavailable
    0x01,
    'N','O','N','E',' ',' ',' ',' ',
    0x00,
  };

  const auto frame = BuildFrame(0x14, payload, sizeof(payload));

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  Feed(dev, info, frame);
  ok1(!info.flarm.traffic.IsEmpty());

  const FlarmId expected = FlarmId::Parse("0A0B0C", nullptr);
  const FlarmTraffic *t = info.flarm.traffic.FindTraffic(expected);
  ok1(t != nullptr);

  ok1(t != nullptr && !t->altitude_available);
  ok1(t != nullptr && !t->speed_received);
  ok1(t != nullptr && !t->climb_rate_received);
  ok1(t != nullptr && !t->track_received);
}

static void
TestOversizeFrameDropped() noexcept
{
  /* Ensure pathological frames don't cause excessive work/allocations. */
  std::vector<uint8_t> payload(2000, 0);
  const auto frame = BuildFrame(0x14, payload.data(), payload.size());

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  Feed(dev, info, frame);

  ok1(!info.alive);
  ok1(info.flarm.traffic.IsEmpty());
}

int main()
{
  plan_tests(89);

  TestBadCrcIgnored();
  TestHeartbeatTimeOfDay();
  TestResyncAfterBadCrc();
  TestNoisePrefix();
  TestTwoFramesOneChunk();
  TestEscapingInPayload();
  TestOwnshipDriver();
  TestOwnshipGeometricAltitude();
  TestOwnshipGeometricAltitudeDatum();
  TestForeFlightAhrs();
  TestTrafficDriver();
  TestTrafficAddressTypeSource();
  TestRelativeAltitudeFromPressureAltitude();
  TestHorizontalRangeFilter();
  TestVerticalRangeFilter();
  TestVerticalFilterSkippedWithoutOwnshipPressure();
  TestChunkedOwnshipFrame();
  TestNegativeCoordinates();
  TestUnavailableFields();
  TestOversizeFrameDropped();

  return exit_status();
}

