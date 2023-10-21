// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "event/SocketEvent.hxx"
#include "net/StaticSocketAddress.hxx"
#include "util/SpanCast.hxx"

#include <chrono>
#include <cstdint>
#include <exception>
#include <span>

struct GeoPoint;

namespace SkyLinesTracking {

/**
 * A server for the SkyLines live tracking protocol.
 *
 * To use this class, derive your class from it and implement the
 * virtual methods.
 */
class Server {
  SocketEvent socket;

public:
  struct Client {
    StaticSocketAddress address;
    uint64_t key;
  };

public:
  Server(EventLoop &event_loop, SocketAddress server_address);

  ~Server();

  constexpr
  static unsigned GetDefaultPort() {
    return 5597;
  }

  constexpr
  static const char *GetDefaultPortString() {
    return "5597";
  }

  auto &GetEventLoop() const noexcept {
    return socket.GetEventLoop();
  }

  void SendBuffer(SocketAddress address,
                  std::span<const std::byte> buffer) noexcept;

  template<typename P>
  void SendPacket(SocketAddress address, const P &packet) noexcept {
    SendBuffer(address, ReferenceAsBytes(packet));
  }

private:
  void OnDatagramReceived(Client &&client, void *data, size_t length);
  void OnSocketReady(unsigned events) noexcept;

protected:
  virtual void OnPing(const Client &client, unsigned id);

  virtual void OnFix([[maybe_unused]] const Client &client,
                     [[maybe_unused]] std::chrono::milliseconds time_of_day,
                     [[maybe_unused]] const ::GeoPoint &location, 
                     [[maybe_unused]] int altitude) {}

  virtual void OnTrafficRequest([[maybe_unused]] const Client &client, [[maybe_unused]] bool near) {}

  virtual void OnUserNameRequest([[maybe_unused]] const Client &client, [[maybe_unused]] uint32_t user_id) {}

  virtual void OnWaveSubmit([[maybe_unused]] const Client &client,
                            [[maybe_unused]] std::chrono::milliseconds time_of_day,
                            [[maybe_unused]] const ::GeoPoint &a,
                            [[maybe_unused]] const ::GeoPoint &b,
                            [[maybe_unused]] int bottom_altitude,
                            [[maybe_unused]] int top_altitude,
                            [[maybe_unused]] double lift) {}

  virtual void OnWaveRequest([[maybe_unused]] const Client &client) {}

  virtual void OnThermalSubmit([[maybe_unused]] const Client &client,
                               [[maybe_unused]] std::chrono::milliseconds time_of_day,
                               [[maybe_unused]] const ::GeoPoint &bottom_location,
                               [[maybe_unused]] int bottom_altitude,
                               [[maybe_unused]] const ::GeoPoint &top_location,
                               [[maybe_unused]] int top_altitude,
                               [[maybe_unused]] double lift) {}

  virtual void OnThermalRequest([[maybe_unused]] const Client &client) {}

  /**
   * An error has occurred while sending a response to a client.  This
   * error is non-fatal.
   */
  virtual void OnSendError([[maybe_unused]] SocketAddress address,
                           [[maybe_unused]] std::exception_ptr e) noexcept {}

  /**
   * An error has occurred, and the SkyLines tracking server is
   * defunct.
   */
  virtual void OnError(std::exception_ptr e) = 0;
};

} /* namespace SkyLinesTracking */
