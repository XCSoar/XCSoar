// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/CRC16CCITT.hpp"
#include "TestUtil.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

static constexpr uint8_t GDL90_FLAG = 0x7e;
static constexpr uint8_t GDL90_ESCAPE = 0x7d;
static constexpr uint8_t GDL90_ESCAPE_XOR = 0x20;

static bool
Unescape(const uint8_t *src, std::size_t size,
         std::vector<uint8_t> &dest) noexcept
{
  dest.clear();
  dest.reserve(size);

  for (std::size_t i = 0; i < size; ++i) {
    uint8_t b = src[i];
    if (b != GDL90_ESCAPE) {
      dest.push_back(b);
      continue;
    }

    if (++i >= size)
      return false;

    dest.push_back(src[i] ^ GDL90_ESCAPE_XOR);
  }

  return true;
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

static constexpr uint16_t
ReadLE16(const uint8_t *p) noexcept
{
  return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

static constexpr uint16_t
ReadBE16(const uint8_t *p) noexcept
{
  return (uint16_t(p[0]) << 8) | uint16_t(p[1]);
}

static constexpr uint32_t
ReadBE24(const uint8_t *p) noexcept
{
  return (uint32_t(p[0]) << 16) | (uint32_t(p[1]) << 8) | uint32_t(p[2]);
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
  return double(value) * (180.0 / double(1u << 23));
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

struct DecodedTraffic {
  uint8_t address_type;
  uint32_t participant_address;
  double lat_deg, lon_deg;
  int altitude_ft;
  char callsign[9];
};

static bool
DecodeTraffic27(const uint8_t *payload, std::size_t size,
                DecodedTraffic &out) noexcept
{
  if (size < 27)
    return false;

  out.address_type = payload[0] >> 4;
  out.participant_address = ReadBE24(payload + 1);

  const int32_t lat_raw = SignExtend24(ReadBE24(payload + 4));
  const int32_t lon_raw = SignExtend24(ReadBE24(payload + 7));
  out.lat_deg = SemiCircles24ToDegrees(lat_raw);
  out.lon_deg = SemiCircles24ToDegrees(lon_raw);

  const uint16_t alt_word = ReadBE16(payload + 10);
  const uint16_t altitude_code = alt_word >> 4;
  if (altitude_code == 0xfff)
    return false;

  out.altitude_ft = int(altitude_code) * 25 - 1000;

  for (unsigned i = 0; i < 8; ++i)
    out.callsign[i] = (char)payload[18 + i];
  out.callsign[8] = 0;

  return true;
}

static void
TestHeartbeatExample() noexcept
{
  /* ICD 2.2.4 example (framed + CRC): 7E 00 81 41 DB D0 08 02 B3 8B 7E
     CRC is little endian: 0x8BB3 (bytes B3 8B) */
  static constexpr uint8_t frame[] = {
    0x7e, 0x00, 0x81, 0x41, 0xdb, 0xd0, 0x08, 0x02, 0xb3, 0x8b, 0x7e,
  };

  std::vector<uint8_t> unescaped;
  ok1(frame[0] == GDL90_FLAG);
  ok1(frame[std::size(frame) - 1] == GDL90_FLAG);

  ok1(Unescape(frame + 1, std::size(frame) - 2, unescaped));
  ok1(unescaped.size() == 1 + 6 + 2);
  ok1(unescaped[0] == 0x00);

  const uint16_t rx_crc = ReadLE16(unescaped.data() + unescaped.size() - 2);
  const uint16_t calc_crc = Gdl90Crc16(unescaped.data(), unescaped.size() - 2);
  ok1(rx_crc == calc_crc);
}

static void
TestTrafficExample() noexcept
{
  /* Example Traffic frame from a known-good implementation (includes one
     escaped byte 0x7D 0x5D -> 0x7D in payload). */
  static constexpr uint8_t frame[] = {
    0x7E, 0x14, 0x00, 0x00, 0x00, 0x00, 0x18, 0x7D, 0x5D, 0xF5,
    0xBD, 0x1F, 0xB4, 0x09, 0x49, 0x88, 0x27, 0x40, 0x00, 0x82,
    0x01, 0x4E, 0x31, 0x32, 0x33, 0x34, 0x35, 0x20, 0x20, 0x00,
    0x5E, 0x66, 0x7E,
  };

  std::vector<uint8_t> unescaped;
  ok1(Unescape(frame + 1, std::size(frame) - 2, unescaped));

  const uint16_t rx_crc = ReadLE16(unescaped.data() + unescaped.size() - 2);
  const uint16_t calc_crc = Gdl90Crc16(unescaped.data(), unescaped.size() - 2);
  ok1(rx_crc == calc_crc);

  ok1(unescaped[0] == 0x14);
  const uint8_t *payload = unescaped.data() + 1;
  const std::size_t payload_len = unescaped.size() - 1 - 2;

  DecodedTraffic t{};
  ok1(DecodeTraffic27(payload, payload_len, t));

  ok1(between(t.lat_deg, 34.0, 35.0));
  ok1(between(t.lon_deg, -95.0, -93.0));
  ok1(t.altitude_ft == 2700);
}

static void
TestTrafficExampleFromIcd() noexcept
{
  /* GDL90 Public ICD Rev A, §3.5.2 / Table 12 "Traffic Report Example".
     This table provides byte values (including Message ID) but not a framed
     example; we compute CRC and build a framed message here. */
  static constexpr uint8_t payload[] = {
    0x00,             // st
    0xAB, 0x45, 0x49, // aa aa aa
    0x1F, 0xEF, 0x15, // ll ll ll
    0xA8, 0x89, 0x78, // nn nn nn
    0x0F,             // dd
    0x09,             // dm
    0xA9,             // ia
    0x07,             // hh
    0xB0,             // hv
    0x01,             // vv
    0x20,             // tt
    0x01,             // ee
    0x4E, 0x38, 0x32, 0x35, 0x56, 0x20, 0x20, 0x20, // cc.. (N825V   )
    0x00,             // px
  };

  const auto frame = BuildFrame(0x14, payload, sizeof(payload));

  std::vector<uint8_t> unescaped;
  ok1(frame.front() == GDL90_FLAG);
  ok1(frame.back() == GDL90_FLAG);
  ok1(Unescape(frame.data() + 1, frame.size() - 2, unescaped));

  const uint16_t rx_crc = ReadLE16(unescaped.data() + unescaped.size() - 2);
  const uint16_t calc_crc = Gdl90Crc16(unescaped.data(), unescaped.size() - 2);
  ok1(rx_crc == calc_crc);

  ok1(unescaped[0] == 0x14);
  const uint8_t *pl = unescaped.data() + 1;
  const std::size_t pl_len = unescaped.size() - 1 - 2;

  DecodedTraffic t{};
  ok1(DecodeTraffic27(pl, pl_len, t));

  ok1(t.participant_address == 0xAB4549);
  ok1(between(t.lat_deg, 44.90, 44.92));
  ok1(between(t.lon_deg, -123.01, -122.98));
  ok1(t.altitude_ft == 5000);
  ok1(std::string_view(t.callsign, 5) == "N825V");
}

static void
TestOwnshipExample() noexcept
{
  /* Ownship Report sample frame (framed + CRC).
     Known-good vector from a GDL90 implementation test suite. */
  static constexpr uint8_t frame[] = {
    0x7E, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x15, 0xA7, 0xE5, 0xBA,
    0x47, 0x99, 0x08, 0xC9, 0x88, 0xFF, 0xE0, 0x00, 0x80, 0x01,
    0x4E, 0x31, 0x32, 0x33, 0x34, 0x35, 0x20, 0x20, 0x00, 0x7B,
    0xE5, 0x7E,
  };

  std::vector<uint8_t> unescaped;
  ok1(Unescape(frame + 1, std::size(frame) - 2, unescaped));

  const uint16_t rx_crc = ReadLE16(unescaped.data() + unescaped.size() - 2);
  const uint16_t calc_crc = Gdl90Crc16(unescaped.data(), unescaped.size() - 2);
  ok1(rx_crc == calc_crc);

  ok1(unescaped[0] == 0x0A);
  const uint8_t *payload = unescaped.data() + 1;
  const std::size_t payload_len = unescaped.size() - 1 - 2;

  DecodedTraffic o{};
  ok1(DecodeTraffic27(payload, payload_len, o));

  ok1(between(o.lat_deg, 30.0, 31.0));
  ok1(between(o.lon_deg, -99.0, -98.0));
  ok1(o.altitude_ft == 2500);
}

int main()
{
  plan_tests(4 + 9 + 2 + 9 + 7);

  TestHeartbeatExample();
  TestTrafficExample();
  TestTrafficExampleFromIcd();
  TestOwnshipExample();

  return exit_status();
}

