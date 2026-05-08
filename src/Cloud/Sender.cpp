// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Sender.hpp"
#include "Tracking/SkyLines/Export.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/CRC16CCITT.hpp"

TrafficRecordExtensions
TrafficRecordExtensions::FromOgn(unsigned track_deg, bool track_valid,
                                 unsigned aircraft_type,
                                 uint32_t flarm_id,
                                 bool flarm_valid) noexcept
{
  TrafficRecordExtensions e{};
  if (track_valid && track_deg <= 359)
    e.reserved = uint16_t(0x8000u | (track_deg & 0x1FFu) |
                          ((aircraft_type & 0x1Fu) << 9));

  if (flarm_valid && flarm_id <= 0xFFFFFFu)
    e.reserved2 = 0x80000000u | (flarm_id & 0xFFFFFFu);

  return e;
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
