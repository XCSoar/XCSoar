// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/SkyLines/Server.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "util/ByteOrder.hxx"
#include "net/StaticSocketAddress.hxx"

#include <array>

struct GeoPoint;

class TrafficResponseSender {
  SkyLinesTracking::Server &server;
  const SocketAddress address;

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
                        SocketAddress client_address, uint64_t key)
    :server(_server), address(client_address) {
    data.header.header.magic = ToBE32(SkyLinesTracking::MAGIC);
    data.header.header.type = ToBE16(SkyLinesTracking::Type::TRAFFIC_RESPONSE);
    data.header.header.key = ToBE64(key);

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
  const SocketAddress address;

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
                        SocketAddress client_address, uint64_t key) noexcept
    :server(_server), address(client_address) {
    data.header.header.magic = ToBE32(SkyLinesTracking::MAGIC);
    data.header.header.type = ToBE16(SkyLinesTracking::Type::THERMAL_RESPONSE);
    data.header.header.key = ToBE64(key);

    data.header.reserved1 = 0;
    data.header.reserved2 = 0;
    data.header.reserved3 = 0;
  }

  void Add(SkyLinesTracking::Thermal t);
  void Flush();
};
