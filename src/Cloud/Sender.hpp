// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/SkyLines/Server.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "util/ByteOrder.hxx"
#include "net/StaticSocketAddress.hxx"

#include <array>
#include <cassert>
#include <cstddef>

struct GeoPoint;

namespace CloudSenderTraits {

struct TrafficBatch {
  using Item = SkyLinesTracking::TrafficResponsePacket::Traffic;
  using HeaderPrefix = SkyLinesTracking::TrafficResponsePacket;
  static constexpr SkyLinesTracking::Type PACKET_TYPE =
    SkyLinesTracking::Type::TRAFFIC_RESPONSE;

  static void
  InitHeader(HeaderPrefix &h, uint64_t key) noexcept
  {
    h.header.magic = ToBE32(SkyLinesTracking::MAGIC);
    h.header.type = ToBE16(PACKET_TYPE);
    h.header.key = ToBE64(key);
    h.reserved = 0;
    h.reserved2 = 0;
    h.reserved3 = 0;
  }

  static void
  SetCount(HeaderPrefix &h, unsigned n) noexcept
  {
    h.traffic_count = static_cast<uint8_t>(n);
  }
};

struct ThermalBatch {
  using Item = SkyLinesTracking::Thermal;
  using HeaderPrefix = SkyLinesTracking::ThermalResponsePacket;
  static constexpr SkyLinesTracking::Type PACKET_TYPE =
    SkyLinesTracking::Type::THERMAL_RESPONSE;

  static void
  InitHeader(HeaderPrefix &h, uint64_t key) noexcept
  {
    h.header.magic = ToBE32(SkyLinesTracking::MAGIC);
    h.header.type = ToBE16(PACKET_TYPE);
    h.header.key = ToBE64(key);
    h.reserved1 = 0;
    h.reserved2 = 0;
    h.reserved3 = 0;
  }

  static void
  SetCount(HeaderPrefix &h, unsigned n) noexcept
  {
    h.thermal_count = static_cast<uint8_t>(n);
  }
};

} // namespace CloudSenderTraits

/**
 * Batches SkyLines variable-length TRAFFIC_RESPONSE / THERMAL_RESPONSE
 * datagrams (CRC16, MTU-sized chunks).
 */
template<typename Traits>
class SkyLinesBatchResponseSender {
  SkyLinesTracking::Server &server;
  const SocketAddress address;

  static constexpr size_t MAX_PACKET_BYTES = 1024;
  static constexpr size_t MAX_ITEMS =
    MAX_PACKET_BYTES / sizeof(typename Traits::Item);

  struct Packet {
    typename Traits::HeaderPrefix header;
    std::array<typename Traits::Item, MAX_ITEMS> items;
  } data;

  unsigned n_items = 0;

public:
  SkyLinesBatchResponseSender(SkyLinesTracking::Server &_server,
                              SocketAddress client_address,
                              uint64_t key) noexcept
    :server(_server), address(client_address)
  {
    Traits::InitHeader(data.header, key);
  }

  void
  Add(const typename Traits::Item &item) noexcept
  {
    assert(n_items < MAX_ITEMS);
    data.items[n_items++] = item;

    if (n_items == MAX_ITEMS)
      Flush();
  }

  void
  Flush() noexcept;
};

using ThermalResponseSender =
  SkyLinesBatchResponseSender<CloudSenderTraits::ThermalBatch>;

class TrafficResponseSender
  : public SkyLinesBatchResponseSender<CloudSenderTraits::TrafficBatch> {
public:
  using SkyLinesBatchResponseSender::SkyLinesBatchResponseSender;

  void
  Add(uint32_t pilot_id, uint32_t time, GeoPoint location, int altitude);
};
