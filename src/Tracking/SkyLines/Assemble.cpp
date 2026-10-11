// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Assemble.hpp"
#include "Export.hpp"
#include "Protocol.hpp"
#include "NMEA/MoreData.hpp"
#include "FLARM/Hardware.hpp"
#include "util/ByteOrder.hxx"
#include "util/CRC16CCITT.hpp"
#include "util/SpanCast.hxx"

using namespace std::chrono;

SkyLinesTracking::PingPacket
SkyLinesTracking::MakePing(uint64_t key, uint16_t id)
{
  assert(key != 0);

  PingPacket packet;
  packet.header.magic = ToBE32(MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(Type::PING);
  packet.header.key = ToBE64(key);
  packet.id = ToBE16(id);
  packet.reserved = 0;
  packet.reserved2 = 0;

  packet.header.crc = ToBE16(UpdateCRC16CCITT(ReferenceAsBytes(packet), 0));
  return packet;
}

SkyLinesTracking::ACKPacket
SkyLinesTracking::MakeAck(uint64_t key, uint16_t id, uint32_t flags)
{
  assert(key != 0);

  ACKPacket packet;
  packet.header.magic = ToBE32(MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(Type::ACK);
  packet.header.key = ToBE64(key);
  packet.id = ToBE16(id);
  packet.reserved = 0;
  packet.flags = ToBE32(flags);

  packet.header.crc = ToBE16(UpdateCRC16CCITT(ReferenceAsBytes(packet), 0));
  return packet;
}

SkyLinesTracking::FixPacket
SkyLinesTracking::MakeFix(uint64_t key, uint32_t flags, uint32_t time,
                          ::GeoPoint location, Angle track,
                          double ground_speed, double airspeed,
                          int altitude, double vario, unsigned enl)
{
  assert(key != 0);

  FixPacket packet;
  packet.header.magic = ToBE32(MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(Type::FIX);
  packet.header.key = ToBE64(key);
  packet.flags = 0;

  packet.flags = ToBE32(flags);
  packet.time = ToBE32(time);
  packet.reserved = 0;

  if (location.IsValid())
    packet.location = ExportGeoPoint(location);
  else
    packet.location.latitude = packet.location.longitude = 0;

  packet.track = ToBE16(uint16_t(track.AsBearing().Degrees()));
  packet.ground_speed = ToBE16(uint16_t(ground_speed * 16));
  packet.airspeed = ToBE16(uint16_t(airspeed * 16));
  packet.altitude = ToBE16(altitude);
  packet.vario = ToBE16(int(vario * 256));
  packet.engine_noise_level = ToBE16(enl);

  packet.header.crc = ToBE16(UpdateCRC16CCITT(ReferenceAsBytes(packet), 0));
  return packet;
}

SkyLinesTracking::FixPacket
SkyLinesTracking::ToFix(uint64_t key, const NMEAInfo &basic)
{
  assert(key != 0);
  assert(basic.time_available);

  FixPacket packet;
  packet.header.magic = ToBE32(MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(Type::FIX);
  packet.header.key = ToBE64(key);
  packet.flags = 0;

  packet.time = ToBE32(basic.time.Cast<duration<uint32_t, milliseconds::period>>().count());
  packet.reserved = 0;

  if (basic.location_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_LOCATION);
    packet.location = ExportGeoPoint(basic.location);
  } else
    packet.location.latitude = packet.location.longitude = 0;

  if (basic.track_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_TRACK);
    packet.track = ToBE16(uint16_t(basic.track.AsBearing().Degrees()));
  } else
    packet.track = 0;

  if (basic.ground_speed_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_GROUND_SPEED);
    packet.ground_speed = ToBE16(uint16_t(basic.ground_speed * 16));
  } else
    packet.ground_speed = 0;

  if (basic.airspeed_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_AIRSPEED);
    packet.airspeed = ToBE16(uint16_t(basic.indicated_airspeed * 16));
  } else
    packet.airspeed = 0;

  if (basic.baro_altitude_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_ALTITUDE);
    packet.altitude = ToBE16(int(basic.baro_altitude));
  } else if (basic.gps_altitude_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_ALTITUDE);
    packet.altitude = ToBE16(int(basic.gps_altitude));
  } else
    packet.altitude = 0;

  if (basic.total_energy_vario_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_VARIO);
    packet.vario = ToBE16(int(basic.total_energy_vario * 256));
  } else if (basic.netto_vario_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_VARIO);
    packet.vario = ToBE16(int(basic.netto_vario * 256));
  } else if (basic.noncomp_vario_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_VARIO);
    packet.vario = ToBE16(int(basic.noncomp_vario * 256));
  } else
    packet.vario = 0;

  if (basic.engine_noise_level_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_ENL);
    packet.engine_noise_level = ToBE16(basic.engine_noise_level);
  } else
    packet.engine_noise_level = 0;

  packet.header.crc = ToBE16(UpdateCRC16CCITT(ReferenceAsBytes(packet), 0));
  return packet;
}

SkyLinesTracking::Thermal
SkyLinesTracking::MakeThermal(uint32_t time,
                              ::GeoPoint bottom_location,
                              int bottom_altitude,
                              ::GeoPoint top_location,
                              int top_altitude,
                              double lift)
{
  Thermal thermal;
  thermal.time = time;
  thermal.reserved1 = 0;
  thermal.bottom_location = ExportGeoPoint(bottom_location);
  thermal.top_location = ExportGeoPoint(top_location);
  thermal.bottom_altitude = ToBE16(bottom_altitude);
  thermal.top_altitude = ToBE16(top_altitude);
  thermal.lift = ToBE16(lround(lift * 256));
  thermal.reserved2 = 0;
  return thermal;
}

SkyLinesTracking::ThermalSubmitPacket
SkyLinesTracking::MakeThermalSubmit(uint64_t key, uint32_t time,
                                    ::GeoPoint bottom_location,
                                    int bottom_altitude,
                                    ::GeoPoint top_location,
                                    int top_altitude,
                                    double lift)
{
  ThermalSubmitPacket packet;
  packet.header.magic = ToBE32(MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(Type::THERMAL_SUBMIT);
  packet.header.key = ToBE64(key);
  packet.thermal = MakeThermal(time, bottom_location, bottom_altitude,
                               top_location, top_altitude, lift);
  packet.header.crc = ToBE16(UpdateCRC16CCITT(ReferenceAsBytes(packet), 0));
  return packet;
}

SkyLinesTracking::ThermalRequestPacket
SkyLinesTracking::MakeThermalRequest(uint64_t key)
{
  assert(key != 0);

  ThermalRequestPacket packet;
  packet.header.magic = ToBE32(MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(Type::THERMAL_REQUEST);
  packet.header.key = ToBE64(key);
  packet.flags = 0;
  packet.reserved1 = 0;

  packet.header.crc = ToBE16(UpdateCRC16CCITT(ReferenceAsBytes(packet), 0));
  return packet;
}

SkyLinesTracking::TrafficRequestPacket
SkyLinesTracking::MakeTrafficRequest(uint64_t key, bool followees, bool club,
                                     bool nearby)
{
  assert(key != 0);

  TrafficRequestPacket packet;
  packet.header.magic = ToBE32(MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(Type::TRAFFIC_REQUEST);
  packet.header.key = ToBE64(key);
  packet.flags = ToBE32((followees ? packet.FLAG_FOLLOWEES : 0)
                        | (club ? packet.FLAG_CLUB : 0)
                        | (nearby ? packet.FLAG_NEAR : 0));
  packet.reserved = 0;

  packet.header.crc = ToBE16(UpdateCRC16CCITT(ReferenceAsBytes(packet), 0));
  return packet;
}

SkyLinesTracking::UserNameRequestPacket
SkyLinesTracking::MakeUserNameRequest(uint64_t key, uint32_t user_id)
{
  assert(key != 0);

  UserNameRequestPacket packet;
  packet.header.magic = ToBE32(MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(Type::USER_NAME_REQUEST);
  packet.header.key = ToBE64(key);
  packet.user_id = ToBE32(user_id);
  packet.reserved = 0;

  packet.header.crc = ToBE16(UpdateCRC16CCITT(ReferenceAsBytes(packet), 0));
  return packet;
}

uint32_t
SkyLinesTracking::MakeOGNId(const FlarmHardware &hardware) noexcept
{
  if (!hardware.radio_id.IsDefined() || hardware.radio_id_type == 0)
    return 0;

  return (uint32_t(hardware.aircraft_type & 0xf) << 26) |
    (uint32_t(hardware.radio_id_type & 0x3) << 24) |
    (hardware.radio_id.Value() & 0xffffff);
}

SkyLinesTracking::OGNFixPacket
SkyLinesTracking::ToOGNFix(uint64_t key, const MoreData &basic,
                           uint32_t id, bool opt_in)
{
  OGNFixPacket packet;
  packet.fix = ToFix(key, basic);
  packet.fix.header.crc = 0;
  packet.fix.flags |= ToBE32(FixPacket::FLAG_OGN);

  uint16_t flags = 0;
  if (opt_in)
    flags |= FixOGNExtension::FLAG_OPT_IN;

  packet.ogn.id = ToBE32(id);
  packet.ogn.reserved[0] = packet.ogn.reserved[1] =
    packet.ogn.reserved[2] = 0;

  if (basic.gps_altitude_available) {
    flags |= FixOGNExtension::FLAG_GPS_ALTITUDE;
    packet.ogn.gps_altitude = ToBE16(int(basic.gps_altitude));
  } else
    packet.ogn.gps_altitude = 0;

  /* not total energy compensated: what the aircraft climbs, like the
     climb rate OGN receivers report */
  if (basic.gps_vario_available) {
    flags |= FixOGNExtension::FLAG_VERTICAL_SPEED;
    packet.ogn.vertical_speed = ToBE16(int(basic.gps_vario * 256));
  } else
    packet.ogn.vertical_speed = 0;

  packet.ogn.flags = ToBE16(flags);

  packet.fix.header.crc =
    ToBE16(UpdateCRC16CCITT(ReferenceAsBytes(packet), 0));
  return packet;
}
