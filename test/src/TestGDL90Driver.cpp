// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/GDL90/Driver.hpp"
#include "FLARM/Id.hpp"
#include "NMEA/Info.hpp"
#include "TestUtil.hpp"
#include "util/CRC16CCITT.hpp"

#include <cstdint>
#include <vector>

static constexpr uint8_t GDL90_FLAG = 0x7e;
static constexpr uint8_t GDL90_ESCAPE = 0x7d;
static constexpr uint8_t GDL90_ESCAPE_XOR = 0x20;

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
  /* Ownship Report framed vector (same as TestGDL90). */
  static constexpr uint8_t ownship_frame[] = {
    0x7E, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x15, 0xA7, 0xE5, 0xBA,
    0x47, 0x99, 0x08, 0xC9, 0x88, 0xFF, 0xE0, 0x00, 0x80, 0x01,
    0x4E, 0x31, 0x32, 0x33, 0x34, 0x35, 0x20, 0x20, 0x00, 0x7B,
    0xE5, 0x7E,
  };

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  const auto *p = reinterpret_cast<const std::byte *>(ownship_frame);
  dev.DataReceived({p, sizeof(ownship_frame)}, info);

  ok1(info.alive);
  ok1(info.location_available);
  ok1(between(info.location.latitude.Degrees(), 30.0, 31.0));
  ok1(between(info.location.longitude.Degrees(), -99.0, -98.0));
  ok1(info.pressure_altitude_available);
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
}

static void
TestChunkedOwnshipFrame() noexcept
{
  /* Feed one framed Ownship Report in small chunks to validate buffering. */
  static constexpr uint8_t ownship_frame[] = {
    0x7E, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x15, 0xA7, 0xE5, 0xBA,
    0x47, 0x99, 0x08, 0xC9, 0x88, 0xFF, 0xE0, 0x00, 0x80, 0x01,
    0x4E, 0x31, 0x32, 0x33, 0x34, 0x35, 0x20, 0x20, 0x00, 0x7B,
    0xE5, 0x7E,
  };

  GDL90Device dev;
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  const auto *bytes = reinterpret_cast<const std::byte *>(ownship_frame);
  dev.DataReceived({bytes, 3}, info);
  ok1(!info.location_available);

  dev.DataReceived({bytes + 3, 5}, info);
  ok1(!info.location_available);

  dev.DataReceived({bytes + 8, sizeof(ownship_frame) - 8}, info);
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
  plan_tests(2 + 2 + 2 + 2 + 3 + 5 + 5 + 4 + 4 + 6 + 2);

  TestBadCrcIgnored();
  TestResyncAfterBadCrc();
  TestNoisePrefix();
  TestTwoFramesOneChunk();
  TestEscapingInPayload();
  TestOwnshipDriver();
  TestTrafficDriver();
  TestChunkedOwnshipFrame();
  TestNegativeCoordinates();
  TestUnavailableFields();
  TestOversizeFrameDropped();

  return exit_status();
}

