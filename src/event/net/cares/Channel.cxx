// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Channel.hxx"
#include "Error.hxx"
#include "Handler.hxx"
#include "event/SocketEvent.hxx"
#include "net/SocketAddress.hxx"
#include "time/Convert.hxx"
#include "util/Cancellable.hxx"

#include <assert.h>
#include <memory>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#endif

namespace Cares {

class Channel::Socket {
  Channel &channel;
  SocketEvent event;

public:
  Socket(Channel &_channel, SocketDescriptor fd, unsigned events) noexcept
      : channel(_channel), event(channel.GetEventLoop(), BIND_THIS_METHOD(OnSocket), fd) 
  {
    event.Schedule(events);
  }

  SocketDescriptor GetSocket() const noexcept { return event.GetSocket(); }
  void Reschedule(unsigned events) noexcept { event.Schedule(events); }

private:
  void OnSocket(unsigned events) noexcept {
    channel.OnSocket(event.GetSocket(), events);
  }
};

void Channel::sock_state_cb(void *data, ares_socket_t socket_fd, int readable,
                            int writable)
{
  auto &channel = *static_cast<Channel *>(data);
  unsigned new_events = 0;
  if (readable) new_events |= SocketEvent::READ;
  if (writable) new_events |= SocketEvent::WRITE;

  auto prev = channel.sockets.before_begin();
  for (auto it = channel.sockets.begin(); it != channel.sockets.end();) {
    if (it->GetSocket().Get() == socket_fd) {
      if (new_events == 0) {
        channel.sockets.erase_after(prev);
      } else {
        it->Reschedule(new_events);
      }
      return;
    } else {
      prev = it++;
    }
  }

  if (new_events != 0)
    channel.sockets.emplace_front(channel, SocketDescriptor{socket_fd}, new_events);
}

Channel::Channel(EventLoop &event_loop)
    : defer_update_sockets(event_loop, BIND_THIS_METHOD(UpdateSockets)),
      timeout_event(event_loop, BIND_THIS_METHOD(OnTimeout)) 
{
  struct ares_options options{};
  options.sock_state_cb = &Channel::sock_state_cb;
  options.sock_state_cb_data = this;

  int optmask = ARES_OPT_SOCK_STATE_CB;
  int code = ares_init_options(&channel, &options, optmask);
  if (code != 0) throw Error(code, "ares_init_options() failed");
}

Channel::~Channel() noexcept { ares_destroy(channel); }

void Channel::UpdateSockets() noexcept {
  timeout_event.Cancel();
  struct timeval timeout_buffer;
  const auto *t = ares_timeout(channel, nullptr, &timeout_buffer);
  if (t != nullptr) timeout_event.Schedule(ToSteadyClockDuration(*t));
}

void Channel::OnSocket(SocketDescriptor fd, unsigned events) noexcept {
  ares_socket_t read_fd = ARES_SOCKET_BAD, write_fd = ARES_SOCKET_BAD;
  if (events & (SocketEvent::READ | SocketEvent::IMPLICIT_FLAGS)) read_fd = fd.Get();
  if (events & (SocketEvent::WRITE | SocketEvent::IMPLICIT_FLAGS)) write_fd = fd.Get();
  ares_process_fd(channel, read_fd, write_fd);
  ScheduleUpdateSockets();
}

void Channel::OnTimeout() noexcept {
  ares_process_fd(channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
  ScheduleUpdateSockets();
}

template <typename F>
static void AsSocketAddress(const ares_addrinfo_node *node, F &&f) {
  switch (node->ai_family) {
  case AF_INET:
  case AF_INET6:
    f(SocketAddress(node->ai_addr, node->ai_addrlen));
    break;
  default:
    break; // ignore unsupported families
  }
}

class Channel::Request final : public Cancellable, public std::enable_shared_from_this<Request> {
  Handler *handler;
  unsigned pending;
  bool success = false;

  std::shared_ptr<Request> self; // keeps object alive during async ops

public:
  Request(Handler &_handler, unsigned _pending, CancellablePointer &cancel_ptr)
      : handler(&_handler), pending(_pending) 
  {
    assert(pending > 0);
    cancel_ptr = *this;
  }

  void SetSelf(std::shared_ptr<Request> s) noexcept { self = std::move(s); }

  void Start(ares_channel _channel, const char *name, int family) noexcept {
    const struct ares_addrinfo_hints hints{ARES_AI_NOSORT, family, 0, 0};
    ares_getaddrinfo(_channel, name, nullptr, &hints, HostCallback, this);
  }

private:
  void Cancel() noexcept override {
    assert(handler != nullptr);
    assert(pending > 0);
    handler = nullptr;
    // Do not decrement pending here; let HostCallback handle it
  }

  /**
   * @brief Handle ares_getaddrinfo completion for this Request.
   *
   * Processes the DNS callback result, delivers zero or more resolved addresses to the
   * associated Handler, and signals final success or error when all pending lookups complete.
   *
   * @param status ARES status code for the completed lookup.
   * @param addressinfo Pointer to resolved address information (may be null).
   */
  void HostCallback(int status, struct ares_addrinfo *addressinfo) noexcept {
    std::unique_ptr<ares_addrinfo, decltype(&ares_freeaddrinfo)> info(addressinfo, ares_freeaddrinfo);

    const bool was_cancelled = !handler;

    // Always decrement pending when a callback fires
    if (was_cancelled) {
      if (--pending == 0)
        self.reset();
      return;
    }

    if (status != ARES_SUCCESS) {
      if (!success && --pending == 0) {
        handler->OnCaresError(std::make_exception_ptr(Error(status, "ares_getaddrinfo() failed")));
        self.reset();
      } else if (success && --pending == 0) {
        handler->OnCaresSuccess();
        self.reset();
      } else {
        // Had error but still waiting for other callback
        // pending already decremented above
      }
      return;
    }

    if (addressinfo) {
      success = true;
      for (auto node = addressinfo->nodes; node; node = node->ai_next)
        AsSocketAddress(node, [handler = handler](SocketAddress addr) { handler->OnCaresAddress(addr); });
      if (--pending == 0) {
        handler->OnCaresSuccess();
        self.reset();
      }
      return;
    } else {
      // Treat null addressinfo as DNS error, even if status == ARES_SUCCESS
      if (--pending == 0) {
        if (!success) {
          handler->OnCaresError(std::make_exception_ptr(Error(status, "ares_getaddrinfo() returned no addresses")));
        } else {
          handler->OnCaresSuccess();
        }
        self.reset();
      }
      return;
    }
  }

  static void HostCallback(void *arg, int status, int, struct ares_addrinfo *result) noexcept {
    auto &request = *static_cast<Request *>(arg);
    request.HostCallback(status, result);
  }
};

void Channel::Lookup(const char *name, int family, Handler &handler,
                     CancellablePointer &cancel_ptr) noexcept {
  auto req = std::make_shared<Request>(handler, 1, cancel_ptr);
  req->SetSelf(req);
  req->Start(channel, name, family);
  UpdateSockets();
}

void Channel::Lookup(const char *name, Handler &handler,
                     CancellablePointer &cancel_ptr) noexcept {
  auto req = std::make_shared<Request>(handler, 2, cancel_ptr);
  req->SetSelf(req);
  req->Start(channel, name, AF_INET6);
  req->Start(channel, name, AF_INET);
  UpdateSockets();
}

} // namespace Cares
