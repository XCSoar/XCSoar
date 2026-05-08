// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OGNClient.hpp"

#include "event/Loop.hxx"
#include "event/net/cares/Channel.hxx"
#include "net/SocketAddress.hxx"
#include "util/BindMethod.hxx"
#include "util/SpanCast.hxx"

#include <array>
#include <chrono>
#include <cstdio>

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
  BeginLookup();
}

void
OGNClient::Stop() noexcept
{
  reconnect_timer.Cancel();
  resolver_job.reset();
  CloseConnection();
  connector.Cancel();
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
    connector.Cancel();
    if (connector.Connect(a, std::chrono::seconds(15)))
      return;
  }

  ScheduleReconnect();
}

void
OGNClient::OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept
{
  read_event.Open(fd.Release());
  read_event.ScheduleRead();
  rx_buffer.clear();
  SendLogin();
}

void
OGNClient::OnSocketConnectError([[maybe_unused]] std::exception_ptr error) noexcept
{
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
  connector.Cancel();
}

void
OGNClient::ScheduleReconnect() noexcept
{
  CloseConnection();
  reconnect_timer.Schedule(std::chrono::seconds(25));
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
    ScheduleReconnect();
    return;
  }

  if (events & SocketEvent::ERROR) {
    ScheduleReconnect();
    return;
  }

  std::array<std::byte, 4096> buf{};
  const ssize_t nbytes =
    read_event.GetSocket().Read(std::span<std::byte>{buf.data(), buf.size()});
  if (nbytes <= 0) {
    ScheduleReconnect();
    return;
  }

  ConsumeInput({(const char *)buf.data(), (std::size_t)nbytes});
}

void
OGNClient::ConsumeInput(std::string_view chunk) noexcept
{
  rx_buffer.append(chunk.data(), chunk.size());

  while (true) {
    const auto nl = rx_buffer.find('\n');
    if (nl == std::string::npos)
      break;

    std::string_view line(rx_buffer.data(), nl);
    while (!line.empty() && line.back() == '\r')
      line.remove_suffix(1);

    handler.OnAprsLine(line);

    rx_buffer.erase(0, nl + 1);
  }

  static constexpr std::size_t MAX_ACCUM = 256 * 1024;
  if (rx_buffer.size() > MAX_ACCUM)
    rx_buffer.clear();
}
