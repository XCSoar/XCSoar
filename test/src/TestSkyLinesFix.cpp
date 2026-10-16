// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * The fix a client sends to SkyLines or to the XCSoar Cloud, and how
 * the server reads it back (#1282).
 */

#include "Tracking/SkyLines/Assemble.hpp"
#include "Tracking/SkyLines/Import.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "NMEA/MoreData.hpp"
#include "FLARM/Hardware.hpp"
#include "util/ByteOrder.hxx"
#include "util/CRC16CCITT.hpp"
#include "util/SpanCast.hxx"
#include "TestUtil.hpp"

using namespace SkyLinesTracking;

static constexpr uint64_t KEY = 0x123456789abcdef0;

static MoreData
MakeBasic() noexcept
{
  MoreData basic;
  basic.Reset();
  basic.clock = TimeStamp{FloatDuration{100}};
  basic.time = TimeStamp{std::chrono::hours{13}};
  basic.time_available.Update(basic.clock);
  basic.location = ::GeoPoint(Angle::Degrees(11.5), Angle::Degrees(48.5));
  basic.location_available.Update(basic.clock);
  basic.track = Angle::Degrees(270);
  basic.track_available.Update(basic.clock);
  basic.ground_speed = 30;
  basic.ground_speed_available.Update(basic.clock);
  basic.baro_altitude = 1500;
  basic.baro_altitude_available.Update(basic.clock);
  basic.gps_altitude = 1540;
  basic.gps_altitude_available.Update(basic.clock);
  basic.total_energy_vario = 2.5;
  basic.total_energy_vario_available.Update(basic.clock);
  basic.gps_vario = 1.75;
  basic.gps_vario_available.Update(basic.clock);
  return basic;
}

template<typename P>
static bool
CheckCRC(P packet) noexcept
{
  const uint16_t crc = FromBE16(packet.fix.header.crc);
  packet.fix.header.crc = 0;
  return crc == UpdateCRC16CCITT(ReferenceAsBytes(packet), 0);
}

/**
 * SkyLines rejects a fix which is not exactly a #FixPacket, so the
 * plain fix must not change.
 */
static void
TestSkyLinesFix()
{
  const FixPacket packet = ToFix(KEY, MakeBasic());
  ok1(sizeof(packet) == 48);
  ok1(!(FromBE32(packet.flags) & FixPacket::FLAG_OGN));
  /* the baro altitude and the total energy vario, as before */
  ok1(FromBE16(packet.altitude) == 1500);
  ok1(FromBE16(packet.vario) == 2.5 * 256);

  const auto fix = ImportFix(ReferenceAsBytes(packet));
  ok1(fix.has_value());
  ok1(fix && fix->altitude == 1500);
  ok1(fix && fix->track_valid && fix->track_deg == 270);
  ok1(fix && fix->ground_speed && *fix->ground_speed == 30);
  ok1(fix && !fix->ogn);
}

static void
TestOGNFix()
{
  /* FLARM address DD1234, address type FLARM, aircraft type glider */
  static constexpr uint32_t id = (1u << 26) | (2u << 24) | 0xdd1234;

  const OGNFixPacket packet = ToOGNFix(KEY, MakeBasic(), id, true);
  ok1(sizeof(packet) == 64);
  ok1(FromBE32(packet.fix.flags) & FixPacket::FLAG_OGN);
  ok1(CheckCRC(packet));

  const auto fix = ImportFix(ReferenceAsBytes(packet));
  ok1(fix && fix->altitude == 1500);
  ok1(fix && fix->ogn.has_value());
  ok1(fix && fix->ogn && fix->ogn->id == id);
  ok1(fix && fix->ogn && fix->ogn->opt_in);
  ok1(fix && fix->ogn && fix->ogn->gps_altitude == 1540);
  /* the GPS vario, not the total energy vario */
  ok1(fix && fix->ogn && fix->ogn->vertical_speed == 1.75);

  /* without consent, and without GPS altitude and vario */
  MoreData basic = MakeBasic();
  basic.gps_altitude_available.Clear();
  basic.gps_vario_available.Clear();
  const auto fix2 = ImportFix(ReferenceAsBytes(ToOGNFix(KEY, basic, 0,
                                                        false)));
  ok1(fix2 && fix2->ogn && !fix2->ogn->opt_in);
  ok1(fix2 && fix2->ogn && fix2->ogn->id == 0);
  ok1(fix2 && fix2->ogn && !fix2->ogn->gps_altitude);
  ok1(fix2 && fix2->ogn && !fix2->ogn->vertical_speed);
}

/**
 * The OGN id from what the FLARM told about itself.
 */
static void
TestOGNId()
{
  FlarmHardware hardware;
  hardware.Clear();
  ok1(MakeOGNId(hardware) == 0);

  hardware.radio_id = FlarmId::FromValue(0xdd1234);
  ok1(MakeOGNId(hardware) == 0);

  hardware.radio_id_type = 2;
  hardware.aircraft_type = 1;
  ok1(MakeOGNId(hardware) == ((1u << 26) | (2u << 24) | 0xdd1234));

  /* an ICAO address of a tow plane */
  hardware.radio_id = FlarmId::FromValue(0x3e4f21);
  hardware.radio_id_type = 1;
  hardware.aircraft_type = 2;
  ok1(MakeOGNId(hardware) == ((2u << 26) | (1u << 24) | 0x3e4f21));
}

/**
 * A server reads only what is there: a fix that claims the extension
 * but is too short to hold it, and a datagram shorter than a fix.
 */
static void
TestShortDatagrams()
{
  const OGNFixPacket packet = ToOGNFix(KEY, MakeBasic(), 0, false);
  const auto bytes = ReferenceAsBytes(packet);

  const auto fix = ImportFix(bytes.first(sizeof(FixPacket)));
  ok1(fix.has_value());
  ok1(fix && !fix->ogn);

  ok1(!ImportFix(bytes.first(sizeof(FixPacket) - 1)));
}

int
main()
{
  plan_tests(9 + 13 + 4 + 3);

  TestSkyLinesFix();
  TestOGNFix();
  TestOGNId();
  TestShortDatagrams();

  return exit_status();
}
