/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Client.hpp"
#include "Protocol.hpp"
#include "OS/ByteOrder.hpp"
#include "NMEA/Info.hpp"
#include "Util/CRC.hpp"

bool
SkyLinesTracking::Client::SendFix(const NMEAInfo &basic)
{
  assert(basic.time_available);

  if (key == 0 || !socket.IsDefined())
    return false;

  FixPacket packet;
  packet.header.magic = ToBE32(MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(Type::FIX);
  packet.header.key = ToBE64(key);
  packet.flags = 0;

  packet.time = ToBE32(unsigned(basic.time * 1000));
  packet.reserved = 0;

  if (basic.location_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_LOCATION);
    ::GeoPoint location = basic.location;
    location.Normalize();
    packet.location.latitude = ToBE32(int(location.latitude.Degrees() * 1000000));
    packet.location.longitude = ToBE32(int(location.longitude.Degrees() * 1000000));
  } else
    packet.location.latitude = packet.location.longitude = 0;

  if (basic.track_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_TRACK);
    packet.track = ToBE16(unsigned(basic.track.AsBearing().Degrees()));
  } else
    packet.track = 0;

  if (basic.ground_speed_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_GROUND_SPEED);
    packet.ground_speed = ToBE16(unsigned(basic.ground_speed * 16));
  } else
    packet.ground_speed = 0;

  if (basic.airspeed_available) {
    packet.flags |= ToBE32(FixPacket::FLAG_AIRSPEED);
    packet.airspeed = ToBE16(unsigned(basic.indicated_airspeed * 16));
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

  packet.header.crc = ToBE16(UpdateCRC16CCITT(&packet, sizeof(packet), 0));

  return socket.Write(&packet, sizeof(packet), address) == sizeof(packet);
}

bool
SkyLinesTracking::Client::SendPing(uint16_t id)
{
  if (key == 0 || !socket.IsDefined())
    return false;

  PingPacket packet;
  packet.header.magic = ToBE32(MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(Type::PING);
  packet.header.key = ToBE64(key);
  packet.id = ToBE16(id);
  packet.reserved = 0;
  packet.reserved2 = 0;

  packet.header.crc = ToBE16(UpdateCRC16CCITT(&packet, sizeof(packet), 0));

  return socket.Write(&packet, sizeof(packet), address) == sizeof(packet);
}
