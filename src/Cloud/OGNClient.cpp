// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OGNClient.hpp"

#include "event/Loop.hxx"
#include "event/net/cares/Channel.hxx"
#include "net/SocketAddress.hxx"
#include "util/BindMethod.hxx"
#include "util/SpanCast.hxx"
#include "util/PrintException.hxx"
#include "util/Exception.hxx"

#include <array>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>

namespace {
constexpr auto CONNECT_TIMEOUT = std::chrono::seconds(15);
constexpr auto RECONNECT_DELAY = std::chrono::seconds(25);
constexpr std::size_t RX_BUFFER_CAPACITY = 16384;

const char *
EnvOr(const char *key, const char *fallback) noexcept
{
  const char *v = std::getenv(key);
  return (v != nullptr && v[0] != '\0') ? v : fallback;
}

bool
DebugEnabled() noexcept
{
  return std::strcmp(EnvOr("XCS_CLOUD_DEBUG", "0"), "1") == 0;
}

} // namespace

OGNClient::OGNClient(EventLoop &_loop, Cares::Channel &_cares,
                     OGNAprsHandler &_handler,
                     std::string &&_host, unsigned _port,
                     std::string &&_user, std::string &&_pass) noexcept
  : ConnectSocketHandler(),
    loop(_loop),
    cares(_cares),
    handler(_handler),
    host(std::move(_host)),
    port(_port),
    user(std::move(_user)),
    pass(std::move(_pass)),
    resolver_handler(*this),
    connector(loop, *this),
    read_event(loop, BIND_THIS_METHOD(OnReadReady)),
    reconnect_timer(loop, BIND_THIS_METHOD(OnReconnectTimer)) {}

void
OGNClient::Start() noexcept
{
  std::cerr << "OGN\tlookup\t" << host << ':' << port << std::endl;
  BeginLookup();
}

void
OGNClient::Stop() noexcept
{
  reconnect_timer.Cancel();
  resolver_job.reset();
  CloseConnection();
}

void
OGNClient::BeginLookup() noexcept
{
  if (resolver_job.has_value())
    return;

  resolver_job.emplace(resolver_handler, port);
  resolver_job->Start(cares, host.c_str());
}

void
OGNClient::TryConnect(std::forward_list<AllocatedSocketAddress> addresses) noexcept
{
  for (AllocatedSocketAddress &a : addresses) {
    if (connector.IsPending())
      connector.Cancel();

    if (connector.Connect(a, CONNECT_TIMEOUT))
      return;
  }

  ScheduleReconnect();
}

void
OGNClient::OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept
{
  std::cerr << "OGN\tconnected\t" << host << ':' << port << std::endl;

  read_event.Open(fd.Release());
  read_event.ScheduleRead();
  rx_buffer.clear();
  try {
    rx_buffer.reserve(RX_BUFFER_CAPACITY);
  } catch (...) {
    std::cerr << "OGN\talloc-error\t" << host << ':' << port << std::endl;
    ScheduleReconnect();
    return;
  }
  SendLogin();
}

void
OGNClient::OnSocketConnectError(std::exception_ptr error) noexcept
{
  std::cerr << "OGN\tconnect-error\t" << host << ':' << port << '\t'
            << GetFullMessage(error) << std::endl;
  ScheduleReconnect();
}

void
OGNClient::SendLogin() noexcept
{
  char line[512];
  const int n =
    std::snprintf(line, sizeof(line),
                  "user %s pass %s vers XCSoar-Cloud 0.1 filter t/o\r\n",
                  user.c_str(), pass.c_str());
  if (n <= 0 || unsigned(n) >= sizeof(line))
    return;

  (void)read_event.GetSocket().WriteNoWait(
    AsBytes(std::string_view(line, unsigned(n))));
}

void
OGNClient::CloseConnection() noexcept
{
  read_event.Close();

  if (connector.IsPending())
    connector.Cancel();
}

void
OGNClient::ScheduleReconnect() noexcept
{
  std::cerr << "OGN\treconnect\t" << host << ':' << port
            << "\tin=" << RECONNECT_DELAY.count() << "s" << std::endl;
  CloseConnection();
  reconnect_timer.Schedule(RECONNECT_DELAY);
}

void
OGNClient::OnReconnectTimer() noexcept
{
  BeginLookup();
}

void
OGNClient::OnReadReady(unsigned events) noexcept
{
  if (events & SocketEvent::HANGUP) {
    std::cerr << "OGN\thangup\t" << host << ':' << port << std::endl;
    ScheduleReconnect();
    return;
  }

  if (events & SocketEvent::ERROR) {
    std::cerr << "OGN\tsocket-error\t" << host << ':' << port << std::endl;
    ScheduleReconnect();
    return;
  }

  std::array<std::byte, 4096> buf{};
  const ssize_t nbytes =
    read_event.GetSocket().Read(std::span<std::byte>{buf.data(), buf.size()});
  if (nbytes <= 0) {
    std::cerr << "OGN\teof\t" << host << ':' << port << std::endl;
    ScheduleReconnect();
    return;
  }

  if (DebugEnabled())
    std::cerr << "OGN\trx\t" << nbytes << " bytes" << std::endl;

  try {
    ConsumeInput({(const char *)buf.data(), (std::size_t)nbytes});
  } catch (...) {
    std::cerr << "OGN\talloc-error\t" << host << ':' << port << std::endl;
    ScheduleReconnect();
  }
}

void
OGNClient::ConsumeInput(std::string_view chunk)
{
  rx_buffer.append(chunk.data(), chunk.size());

  std::size_t cut = 0;
  while (true) {
    const auto nl = rx_buffer.find('\n', cut);
    if (nl == std::string::npos)
      break;

    std::string_view line(rx_buffer.data() + cut, nl - cut);
    while (!line.empty() && line.back() == '\r')
      line.remove_suffix(1);

    handler.OnAprsLine(line);
    cut = nl + 1;
  }

  if (cut > 0)
    rx_buffer.erase(0, cut);

  static constexpr std::size_t MAX_ACCUM = 256 * 1024;
  if (rx_buffer.size() > MAX_ACCUM)
    rx_buffer.clear();
}
