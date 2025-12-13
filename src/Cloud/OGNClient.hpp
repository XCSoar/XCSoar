// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Traffic.hpp"
#include "Geo/GeoPoint.hpp"
#include "event/CoarseTimerEvent.hxx"
#include "event/SocketEvent.hxx"
#include "event/net/ConnectSocket.hxx"
#include "event/net/cares/SimpleResolver.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "util/StaticString.hxx"
#include "util/tstring.hpp"

#include <memory>
#include <optional>
#include <string>

class EventLoop;
class OGNTrafficContainer;
namespace Cares {
class Channel;
}

// Forward declaration for testing
#ifdef TEST_OGN_PARSER
class TestOGNHelper;
#endif

/**
 * Client for connecting to OGN APRS-IS servers and parsing traffic data.
 */
class OGNClient final : public Cares::SimpleHandler,
                        public ConnectSocketHandler {
#ifdef TEST_OGN_PARSER
  friend class TestOGNHelper;
#endif
  EventLoop &event_loop;
  OGNTrafficContainer &traffic_container;
  Cares::Channel *cares_channel = nullptr;

  std::optional<Cares::SimpleResolver> resolver;
  std::optional<ConnectSocket> connect;
  AllocatedSocketAddress address;
  std::forward_list<AllocatedSocketAddress> resolved_addresses;
  std::forward_list<AllocatedSocketAddress>::const_iterator current_address;
  SocketEvent socket_event;

  std::string input_buffer;
  CoarseTimerEvent reconnect_timer;
  CoarseTimerEvent
      keepalive_timer; // Send keepalives every 240s (as per python-ogn-client)

  bool enabled = false;
  const char *server = nullptr;
  unsigned port = 0;

public:
  OGNClient(EventLoop &_event_loop, Cares::Channel &_cares,
            OGNTrafficContainer &_traffic_container);
  ~OGNClient();

  auto &GetEventLoop() const noexcept { return event_loop; }

  void SetEnabled(bool _enabled)
  {
    enabled = _enabled;
    if (!enabled) Close();
    else if (server != nullptr) Open();
  }

  void SetServer(const char *_server, unsigned _port)
  {
    server = _server;
    port = _port;
    if (enabled) Open();
  }

  void OnKeepaliveTimer() noexcept;

  bool IsConnected() const { return socket_event.IsDefined(); }

  void Open();
  void Close();

private:
  void OnSocketReady(unsigned events) noexcept;

  /* virtual methods from Cares::SimpleHandler */
  void OnResolverSuccess(
      std::forward_list<AllocatedSocketAddress> addresses) noexcept override;
  void OnResolverError(std::exception_ptr error) noexcept override;

  /* virtual methods from ConnectSocketHandler */
  void OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept override;
  void OnSocketConnectError(std::exception_ptr ep) noexcept override;

  void OnReconnectTimer() noexcept;
  void TryNextAddress() noexcept;

  void ProcessLine(const char *line);
  void ParseAPRSPacket(const char *packet, const tstring &device_id);
  bool ParseOGNPosition(const char *packet, GeoPoint &location, int &altitude,
                        unsigned &track, unsigned &speed, int &climb_rate,
                        double &turn_rate,
                        FlarmTraffic::AircraftType &aircraft_type,
                        bool &track_received);
};
