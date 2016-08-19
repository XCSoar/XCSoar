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

#ifndef XCSOAR_CLOUD_SENDER_HPP
#define XCSOAR_CLOUD_SENDER_HPP

#include "Tracking/SkyLines/Server.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "OS/ByteOrder.hpp"

#include <boost/asio/ip/udp.hpp>

struct GeoPoint;

class TrafficResponseSender {
  SkyLinesTracking::Server &server;
  const boost::asio::ip::udp::endpoint &endpoint;

  static constexpr size_t MAX_TRAFFIC_SIZE = 1024;
  static constexpr size_t MAX_TRAFFIC =
    MAX_TRAFFIC_SIZE / sizeof(SkyLinesTracking::TrafficResponsePacket::Traffic);

  struct Packet {
    SkyLinesTracking::TrafficResponsePacket header;
    std::array<SkyLinesTracking::TrafficResponsePacket::Traffic, MAX_TRAFFIC> traffic;
  } data;

  unsigned n_traffic = 0;

public:
  TrafficResponseSender(SkyLinesTracking::Server &_server,
                        const SkyLinesTracking::Server::Client &client)
    :server(_server), endpoint(client.endpoint) {
    data.header.header.magic = ToBE32(SkyLinesTracking::MAGIC);
    data.header.header.type = ToBE16(SkyLinesTracking::Type::TRAFFIC_RESPONSE);
    data.header.header.key = ToBE64(client.key);

    data.header.reserved = 0;
    data.header.reserved2 = 0;
    data.header.reserved3 = 0;
  }

  void Add(uint32_t pilot_id, uint32_t time,
           GeoPoint location, int altitude);
  void Flush();
};

class ThermalResponseSender {
  SkyLinesTracking::Server &server;
  const boost::asio::ip::udp::endpoint &endpoint;

  static constexpr size_t MAX_THERMAL_SIZE = 1024;
  static constexpr size_t MAX_THERMAL =
    MAX_THERMAL_SIZE / sizeof(SkyLinesTracking::Thermal);

  struct Packet {
    SkyLinesTracking::ThermalResponsePacket header;
    std::array<SkyLinesTracking::Thermal, MAX_THERMAL> thermal;
  } data;

  unsigned n_thermal = 0;

public:
  ThermalResponseSender(SkyLinesTracking::Server &_server,
                        const SkyLinesTracking::Server::Client &client)
    :server(_server), endpoint(client.endpoint) {
    data.header.header.magic = ToBE32(SkyLinesTracking::MAGIC);
    data.header.header.type = ToBE16(SkyLinesTracking::Type::THERMAL_RESPONSE);
    data.header.header.key = ToBE64(client.key);

    data.header.reserved1 = 0;
    data.header.reserved2 = 0;
    data.header.reserved3 = 0;
  }

  void Add(SkyLinesTracking::Thermal t);
  void Flush();
};

#endif
