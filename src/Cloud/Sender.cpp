/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Sender.hpp"
#include "Tracking/SkyLines/Export.hpp"
#include "Geo/GeoPoint.hpp"
#include "Util/CRC.hpp"

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
  server.SendBuffer(endpoint, boost::asio::const_buffer(&data, size));
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
  server.SendBuffer(endpoint, boost::asio::const_buffer(&data, size));
}
