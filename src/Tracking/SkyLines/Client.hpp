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

#ifndef XCSOAR_TRACKING_SKYLINES_CLIENT_HPP
#define XCSOAR_TRACKING_SKYLINES_CLIENT_HPP

#include "event/SocketEvent.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "event/net/cares/SimpleResolver.hxx"
#include "thread/Mutex.hxx"
#include "util/Cancellable.hxx"
#include "util/Compiler.h"

#include <cstdint>
#include <optional>

struct NMEAInfo;
struct GeoPoint;

namespace Cares { class Channel; }

namespace SkyLinesTracking {

struct TrafficResponsePacket;
struct UserNameResponsePacket;
struct WaveResponsePacket;
struct ThermalResponsePacket;
class Handler;

class Client final : Cares::SimpleHandler {
  Handler *const handler;

  /**
   * Protects #resolving, #resolver, #socket.
   */
  mutable Mutex mutex;

  uint64_t key = 0;

  std::optional<Cares::SimpleResolver> resolver;

  AllocatedSocketAddress address;
  UniqueSocketDescriptor socket;
  SocketEvent socket_event;

public:
  explicit Client(EventLoop &event_loop,
                  Handler *_handler=nullptr)
    :handler(_handler),
     socket_event(event_loop, BIND_THIS_METHOD(OnSocketReady)) {}
  ~Client() { Close(); }

  auto &GetEventLoop() const noexcept {
    return socket_event.GetEventLoop();
  }

  constexpr
  static unsigned GetDefaultPort() {
    return 5597;
  }

  /**
   * Is SkyLines tracking enabled in configuration?
   */
  bool IsEnabled() const {
    return key != 0;
  }

  bool IsDefined() const {
    const std::lock_guard<Mutex> lock(mutex);
    return resolver || socket.IsDefined();
  }

  gcc_pure
  bool IsConnected() const {
    const std::lock_guard<Mutex> lock(mutex);
    return socket.IsDefined();
  }

  uint64_t GetKey() const {
    return key;
  }

  void SetKey(uint64_t _key) {
    key = _key;
  }

  void Open(Cares::Channel &cares, const char *server);
  bool Open(SocketAddress _address);
  void Close();

  template<typename P>
  bool SendPacket(const P &packet) {
    const std::lock_guard<Mutex> lock(mutex);
    return socket.Write(&packet, sizeof(packet), address) == sizeof(packet);
  }

  void SendFix(const NMEAInfo &basic);
  void SendPing(uint16_t id);

  void SendThermal(uint32_t time,
                   ::GeoPoint bottom_location, int bottom_altitude,
                   ::GeoPoint top_location, int top_altitude,
                   double lift);
  void SendThermalRequest();

  void SendTrafficRequest(bool followees, bool club, bool near_);
  void SendUserNameRequest(uint32_t user_id);

private:
  void InternalClose() noexcept;

  void OnTrafficReceived(const TrafficResponsePacket &packet, size_t length);
  void OnUserNameReceived(const UserNameResponsePacket &packet,
                          size_t length);
  void OnWaveReceived(const WaveResponsePacket &packet, size_t length);
  void OnThermalReceived(const ThermalResponsePacket &packet, size_t length);
  void OnDatagramReceived(void *data, size_t length);

  void OnSocketReady(unsigned events) noexcept;

  /* virtual methods from Cares::SimpleHandler */
  void OnResolverSuccess(std::forward_list<AllocatedSocketAddress> addresses) noexcept override;
  void OnResolverError(std::exception_ptr error) noexcept override;
};

} /* namespace SkyLinesTracking */

#endif
