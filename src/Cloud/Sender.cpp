// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Sender.hpp"
#include "Tracking/SkyLines/Export.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/CRC16CCITT.hpp"

void
TrafficResponseSender::Add(uint32_t pilot_id, uint32_t time,
                           GeoPoint location, int altitude)
{
  assert(n_traffic < MAX_TRAFFIC);

  auto &traffic = data.traffic[n_traffic++];
  traffic.pilot_id = ToBE32(pilot_id);
  traffic.time = ToBE32(time);
  traffic.location = SkyLinesTracking::ExportGeoPoint(location);
  traffic.altitude = ToBE16(altitude);
  traffic.reserved = 0;
  traffic.reserved2 = 0;

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
