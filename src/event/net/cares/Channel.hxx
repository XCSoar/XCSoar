// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "Init.hxx"
#include "event/DeferEvent.hxx"
#include "event/CoarseTimerEvent.hxx"

#include <ares.h>

#include <forward_list>

class CancellablePointer;
class SocketDescriptor;

namespace Cares {

class Handler;

/**
 * C++ wrapper for #ares_channel with #EventLoop integration.
 */
class Channel {
  Init init;

  ares_channel channel;

  DeferEvent defer_update_sockets;
  CoarseTimerEvent timeout_event;

  class Socket;

  std::forward_list<Socket> sockets;

  class Request;

public:
  explicit Channel(EventLoop &event_loop);
  ~Channel() noexcept;

  EventLoop &GetEventLoop() const noexcept
  {
    return defer_update_sockets.GetEventLoop();
  }

  /**
   * Look up a host name and call a #Handler method upon
   * completion.
   */
  void Lookup(const char *name, int family, Handler &handler,
              CancellablePointer &cancel_ptr) noexcept;

  /**
   * This overload submits two queries: AF_INET and AF_INET6 and
   * returns both results to the handler.
   */
  void Lookup(const char *name, Handler &handler,
              CancellablePointer &cancel_ptr) noexcept;

private:
  void UpdateSockets() noexcept;
  void ScheduleUpdateSockets() noexcept { defer_update_sockets.Schedule(); }
  void OnSocket(SocketDescriptor fd, unsigned events) noexcept;
  void OnTimeout() noexcept;
  static void sock_state_cb(void *data, ares_socket_t socket_fd, int readable, int writable);

};

} // namespace Cares
