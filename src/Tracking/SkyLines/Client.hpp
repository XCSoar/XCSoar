// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "event/SocketEvent.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "event/net/cares/SimpleResolver.hxx"
#include "thread/Mutex.hxx"
#include "util/Cancellable.hxx"
#include "util/SpanCast.hxx"

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
    const std::lock_guard lock{mutex};
    return resolver || socket_event.IsDefined();
  }

  [[gnu::pure]]
  bool IsConnected() const {
    const std::lock_guard lock{mutex};
    return socket_event.IsDefined();
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
    const std::lock_guard lock{mutex};
    return GetSocket().WriteNoWait(ReferenceAsBytes(packet), address) == sizeof(packet);
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
  SocketDescriptor GetSocket() noexcept {
    return socket_event.GetSocket();
  }

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
