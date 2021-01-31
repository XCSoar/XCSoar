/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_TRACKING_SKYLINES_SERVER_HPP
#define XCSOAR_TRACKING_SKYLINES_SERVER_HPP

#include "event/SocketEvent.hxx"
#include "net/StaticSocketAddress.hxx"
#include "util/ConstBuffer.hxx"

#include <chrono>
#include <exception>

#include <cstdint>

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

  void SendBuffer(SocketAddress address, ConstBuffer<void> buffer) noexcept;

  template<typename P>
  void SendPacket(SocketAddress address, const P &packet) noexcept {
    SendBuffer(address, ConstBuffer<void>{&packet, sizeof(packet)});
  }

private:
  void OnDatagramReceived(Client &&client, void *data, size_t length);
  void OnSocketReady(unsigned events) noexcept;

protected:
  virtual void OnPing(const Client &client, unsigned id);

  virtual void OnFix(const Client &client,
                     std::chrono::milliseconds time_of_day,
                     const ::GeoPoint &location, int altitude) {}

  virtual void OnTrafficRequest(const Client &client, bool near) {}

  virtual void OnUserNameRequest(const Client &client, uint32_t user_id) {}

  virtual void OnWaveSubmit(const Client &client,
                            std::chrono::milliseconds time_of_day,
                            const ::GeoPoint &a, const ::GeoPoint &b,
                            int bottom_altitude,
                            int top_altitude,
                            double lift) {}

  virtual void OnWaveRequest(const Client &client) {}

  virtual void OnThermalSubmit(const Client &client,
                               std::chrono::milliseconds time_of_day,
                               const ::GeoPoint &bottom_location,
                               int bottom_altitude,
                               const ::GeoPoint &top_location,
                               int top_altitude,
                               double lift) {}

  virtual void OnThermalRequest(const Client &client) {}

  /**
   * An error has occurred while sending a response to a client.  This
   * error is non-fatal.
   */
  virtual void OnSendError(SocketAddress address,
                           std::exception_ptr e) noexcept {}

  /**
   * An error has occurred, and the SkyLines tracking server is
   * defunct.
   */
  virtual void OnError(std::exception_ptr e) = 0;
};

} /* namespace SkyLinesTracking */

#endif
