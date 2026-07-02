// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/SkyLines/Server.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "util/ByteOrder.hxx"
#include "net/StaticSocketAddress.hxx"

#include <array>
#include <cstdint>

struct GeoPoint;
struct OGNTrafficEntry;

/**
 * Packed extensions for #SkyLinesTracking::TrafficResponsePacket::Traffic
 * reserved fields (host byte order here; converted in #TrafficResponseSender).
 *
 * - reserved: bit 15 TRACK_VALID; bits 0-8 ground track [deg]; bits 9-13 aircraft
 *   type (0-31).
 * - reserved2: bit 31 FLARM_VALID; bits 0-23 FLARM address when valid.
 *
 * For OGN traffic with a known FLARM address, pilot_id uses
 * #OGNPilotIdFromFlarm() (0x80000000 | flarm_id).
 */
struct TrafficRecordExtensions {
  uint16_t reserved = 0;
  uint32_t reserved2 = 0;

  static TrafficRecordExtensions FromOgn(unsigned track_deg, bool track_valid,
                                         unsigned aircraft_type,
                                         uint32_t flarm_id,
                                         bool flarm_valid) noexcept;

  static TrafficRecordExtensions
  FromOgn(const OGNTrafficEntry &t) noexcept;
};

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
           GeoPoint location, int altitude,
           TrafficRecordExtensions ext = {});
  void Flush();
};

void
SendUserNameResponse(SkyLinesTracking::Server &server,
                     SocketAddress address, uint64_t key,
                     uint32_t user_id, std::string_view name) noexcept;

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
