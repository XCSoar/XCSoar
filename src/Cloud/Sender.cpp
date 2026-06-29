// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Sender.hpp"
#include "OGNTraffic.hpp"
#include "Tracking/SkyLines/Export.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "Tracking/SkyLines/TrafficExtensions.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/CRC16CCITT.hpp"
#include "util/UTF8.hpp"

#include <array>
#include <cstring>

TrafficRecordExtensions
TrafficRecordExtensions::FromOgn(unsigned track_deg, bool track_valid,
                                 unsigned aircraft_type,
                                 uint32_t flarm_id,
                                 bool flarm_valid,
                                 bool altitude_valid) noexcept
{
  const auto ext = SkyLinesTracking::TrafficExtensions::FromOgn(
    track_deg, track_valid, aircraft_type, flarm_id, flarm_valid,
    altitude_valid);
  const auto wire = ext.ToWire();
  return {wire.reserved, wire.reserved2};
}

TrafficRecordExtensions
TrafficRecordExtensions::FromOgn(const OGNTrafficEntry &t) noexcept
{
  return FromOgn(t.track_deg, t.track_valid, t.aircraft_type,
                 t.flarm_id, t.flarm_valid, t.altitude_valid);
}

void
TrafficResponseSender::Add(uint32_t pilot_id, uint32_t time,
                           GeoPoint location, int altitude,
                           TrafficRecordExtensions ext)
{
  assert(n_traffic < MAX_TRAFFIC);

  auto &traffic = data.traffic[n_traffic++];
  traffic.pilot_id = ToBE32(pilot_id);
  traffic.time = ToBE32(time);
  traffic.location = SkyLinesTracking::ExportGeoPoint(location);
  traffic.altitude = ToBE16(altitude);
  traffic.reserved = ToBE16(ext.reserved);
  traffic.reserved2 = ToBE32(ext.reserved2);

  if (n_traffic == MAX_TRAFFIC)
    Flush();
}

void
TrafficResponseSender::Flush()
{
  if (n_traffic == 0)
    return;

  size_t size = sizeof(data.header) + sizeof(data.traffic[0]) * n_traffic;

  data.header.traffic_count = n_traffic;
  n_traffic = 0;

  data.header.header.crc = 0;
  data.header.header.crc = ToBE16(UpdateCRC16CCITT(&data, size, 0));
  server.SendBuffer(address, {(const std::byte *)&data, size});
}

void
SendUserNameResponse(SkyLinesTracking::Server &server,
                     SocketAddress address, uint64_t key,
                     uint32_t user_id, std::string_view name) noexcept
{
  if (name.empty() || name.size() > 255 || !ValidateUTF8(name))
    return;

  std::array<std::byte, sizeof(SkyLinesTracking::UserNameResponsePacket) + 255>
    buffer{};

  auto &packet = *(SkyLinesTracking::UserNameResponsePacket *)buffer.data();
  packet.header.magic = ToBE32(SkyLinesTracking::MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(SkyLinesTracking::Type::USER_NAME_RESPONSE);
  packet.header.key = ToBE64(key);
  packet.user_id = ToBE32(user_id);
  packet.flags = 0;
  packet.club_id = 0;
  packet.name_length = uint8_t(name.size());
  packet.reserved1 = 0;
  packet.reserved2 = 0;
  packet.reserved3 = 0;
  packet.reserved4 = 0;
  packet.reserved5 = 0;

  std::memcpy(&packet + 1, name.data(), name.size());

  const size_t size = sizeof(packet) + name.size();
  packet.header.crc = ToBE16(UpdateCRC16CCITT(buffer.data(), size, 0));
  server.SendBuffer(address, {buffer.data(), size});
}

void
ThermalResponseSender::Add(SkyLinesTracking::Thermal t)
{
  assert(n_thermal < MAX_THERMAL);

  data.thermal[n_thermal++] = t;

  if (n_thermal == MAX_THERMAL)
    Flush();
}

void
ThermalResponseSender::Flush()
{
  if (n_thermal == 0)
    return;

  size_t size = sizeof(data.header) + sizeof(data.thermal[0]) * n_thermal;

  data.header.thermal_count = n_thermal;
  n_thermal = 0;

  data.header.header.crc = 0;
  data.header.header.crc = ToBE16(UpdateCRC16CCITT(&data, size, 0));
  server.SendBuffer(address, {(const std::byte *)&data, size});
}
