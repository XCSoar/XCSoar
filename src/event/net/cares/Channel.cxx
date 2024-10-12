// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>


#include "Channel.hxx"
#include "Handler.hxx"
#include "Error.hxx"
#include "event/SocketEvent.hxx"
#include "net/SocketAddress.hxx"
#include "time/Convert.hxx"
#include "util/Cancellable.hxx"

#include <assert.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#endif

namespace Cares {

class Channel::Socket {
  Channel &channel;
  SocketEvent event;

public:
  Socket(Channel &_channel, SocketDescriptor fd, unsigned events) noexcept
      : channel(_channel),
        event(channel.GetEventLoop(), BIND_THIS_METHOD(OnSocket), fd)
  {
    event.Schedule(events);
  }

private:
  void OnSocket(unsigned events) noexcept
  {
    event.Cancel();
    channel.OnSocket(event.GetSocket(), events);
  }
};

void Channel::sock_state_cb(void *data, ares_socket_t socket_fd, int readable, int writable)
{
  unsigned events = 0;
    if (readable) events |= SocketEvent::READ;
    if (writable) events |= SocketEvent::WRITE;

    if (events != 0){
      auto &channel= *static_cast<Channel*>(data);
      channel.sockets.emplace_front(channel, SocketDescriptor{socket_fd}, events);
    }
}

Channel::Channel(EventLoop &event_loop)
    : defer_update_sockets(event_loop, BIND_THIS_METHOD(UpdateSockets)),
      timeout_event(event_loop, BIND_THIS_METHOD(OnTimeout))
{
  struct ares_options options;

  options.sock_state_cb = & Channel::sock_state_cb;
  options.sock_state_cb_data = this;

  int optmask = 0;
  optmask |= ARES_OPT_SOCK_STATE_CB;

  int code = ares_init_options(&channel,&options,optmask);
  if (code != 0) throw Error(code, "ares_init() failed");
}

Channel::~Channel() noexcept { ares_destroy(channel); }


void
Channel::UpdateSockets() noexcept
{
  timeout_event.Cancel();

  struct timeval timeout_buffer;
  const auto *t = ares_timeout(channel, nullptr, &timeout_buffer);
  if (t != nullptr) timeout_event.Schedule(ToSteadyClockDuration(*t));
}

void
Channel::OnSocket(SocketDescriptor fd, unsigned events) noexcept
{
  ares_socket_t read_fd = ARES_SOCKET_BAD, write_fd = ARES_SOCKET_BAD;

  if (events & (SocketEvent::READ | SocketEvent::IMPLICIT_FLAGS))
    read_fd = fd.Get();

  if (events & (SocketEvent::WRITE | SocketEvent::IMPLICIT_FLAGS))
    write_fd = fd.Get();

  ares_process_fd(channel, read_fd, write_fd);

  ScheduleUpdateSockets();
}

void
Channel::OnTimeout() noexcept
{
  sockets.clear();
  ares_process_fd(channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
  ScheduleUpdateSockets();
}

template<typename F>
static void
AsSocketAddress(const ares_addrinfo_node* node, F &&f)
{
  switch (node->ai_family)
  {
  case AF_INET:
  {
    f(SocketAddress(node->ai_addr, sizeof(sockaddr_in)));
  }
  break;

  case AF_INET6:
  {
    f(SocketAddress(node->ai_addr, sizeof(sockaddr_in6)));
  }
  break;

  default:
    throw std::runtime_error("Unsupported address type");
  }
}

class Channel::Request final : Cancellable {
  Handler *handler;
  unsigned pending;

  bool success = false;

public:
  Request(Handler &_handler, unsigned _pending, CancellablePointer &cancel_ptr)
      : handler(&_handler), pending(_pending)
  {
    assert(pending > 0);
    cancel_ptr = *this;
  }

  void Start(ares_channel _channel, const char *name, int family) noexcept
  {
    const struct ares_addrinfo_hints hints = { ARES_AI_NOSORT, family, 0, 0 };
    ares_getaddrinfo(_channel, name, NULL, &hints, HostCallback,this);
  }

private:
  void Cancel() noexcept override
  {
    assert(handler != nullptr);
    assert(pending > 0);

    /* c-ares doesn't support cancellation, so we emulate
       it here */
    handler = nullptr;
  }

  void HostCallback(int status, struct ares_addrinfo *addressinfo) noexcept;

  static void HostCallback(void *arg, int status, int,
                            struct ares_addrinfo *result) noexcept
  {
    auto &request = *(Request *)arg;
    request.HostCallback(status, result);
  }
};

inline void
Channel::Request::HostCallback(int status, struct ares_addrinfo *addressinfo) noexcept
{
  assert(pending > 0);

  if (handler == nullptr) /* cancelled */
    return;

  --pending;

  try
  {
    if (status != ARES_SUCCESS)
      throw Error(status, "ares_gethostbyname() failed");
    else if (addressinfo != nullptr)
    {
      success = true;
      for (auto node = addressinfo->nodes;node != nullptr; node = node->ai_next){
        AsSocketAddress(node,
                         [&_handler = *handler](SocketAddress address)
                         { _handler.OnCaresAddress(address); });

      if (pending == 0) handler->OnCaresSuccess();
      }
    }
    else throw std::runtime_error("ares_gethostbyname() failed");
  }
  catch (...)
  {
    if (pending == 0)
    {
      if (success) handler->OnCaresSuccess();
      else handler->OnCaresError(std::current_exception());
    }
    ares_freeaddrinfo(addressinfo);
  }

  ares_freeaddrinfo(addressinfo);
  if (pending == 0) delete this;

}

void
Channel::Lookup(const char *name, int family, Handler &handler,
                CancellablePointer &cancel_ptr) noexcept
{
  auto *request = new Request(handler, 1, cancel_ptr);
  request->Start(channel, name, family);
  UpdateSockets();
}

void
Channel::Lookup(const char *name, Handler &handler,
                CancellablePointer &cancel_ptr) noexcept
{
  auto *request = new Request(handler, 2, cancel_ptr);
  request->Start(channel, name, AF_INET);
  request->Start(channel, name, AF_INET6);
  UpdateSockets();
}

} // namespace Cares
