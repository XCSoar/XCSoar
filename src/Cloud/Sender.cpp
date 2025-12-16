// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Sender.hpp"
#include "Tracking/SkyLines/Export.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/CRC16CCITT.hpp"
#include "FLARM/Traffic.hpp"

#include <cstring>

void
TrafficResponseSender::Add(uint32_t pilot_id, uint32_t time,
                           GeoPoint location, int altitude, uint32_t flarm_id,
                           unsigned track,
                           double turn_rate,
                           FlarmTraffic::AircraftType aircraft_type)
{
  assert(n_traffic < MAX_TRAFFIC);

  auto &traffic = data.traffic[n_traffic++];
  traffic.pilot_id = ToBE32(pilot_id);
  traffic.time = ToBE32(time);
  traffic.location = SkyLinesTracking::ExportGeoPoint(location);
  traffic.altitude = ToBE16(altitude);
  
  // Encode turn_rate as int16_t scaled by 10
  int16_t turn_rate_scaled = (int16_t)(turn_rate * 10.0);
  // Clamp to int16_t range
  if (turn_rate * 10.0 > 32767.0)
    turn_rate_scaled = 32767;
  else if (turn_rate * 10.0 < -32768.0)
    turn_rate_scaled = -32768;
  traffic.reserved = ToBE16(*(uint16_t*)&turn_rate_scaled);
  
  // Encode reserved2:
  // - Lower 8 bits: aircraft_type
  // - Next 9 bits (8-16): track (0-359 degrees)
  // - Upper 16 bits (17-32): flarm_id (reduced from 24 to 16 bits)
  uint8_t type_byte = (uint8_t)aircraft_type;
  uint16_t track_bits = track & 0x1FF;  // 9 bits for track (0-359)
  uint16_t flarm_id_bits = (flarm_id >> 8) & 0xFFFF;  // Upper 16 bits of flarm_id
  uint32_t encoded = ((uint32_t)flarm_id_bits << 17) | ((uint32_t)track_bits << 8) | type_byte;
  traffic.reserved2 = ToBE32(encoded);

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
