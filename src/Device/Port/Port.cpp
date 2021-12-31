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

#include "Port.hpp"
#include "Listener.hpp"
#include "Device/Error.hpp"
#include "time/TimeoutClock.hpp"
#include "Operation/Operation.hpp"
#include "Operation/Cancelled.hpp"
#include "util/Exception.hxx"

#include <algorithm>
#include <cassert>

#include <string.h>

Port::Port(PortListener *_listener, DataHandler &_handler) noexcept
  :listener(_listener), handler(_handler) {}

Port::~Port() noexcept = default;

bool
Port::WaitConnected(OperationEnvironment &env)
{
  while (GetState() == PortState::LIMBO) {
    if (env.IsCancelled())
      throw OperationCancelled{};

    env.Sleep(std::chrono::milliseconds(200));
  }

  return GetState() == PortState::READY;
}

std::size_t
Port::Write(const char *s)
{
  return Write(s, strlen(s));
}

void
Port::FullWrite(const void *buffer, std::size_t length,
                OperationEnvironment &env,
                std::chrono::steady_clock::duration _timeout)
{
  const TimeoutClock timeout(_timeout);

  const char *p = (const char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.HasExpired())
      throw DeviceTimeout{"Port write timeout"};

    std::size_t nbytes = Write(p, end - p);
    assert(nbytes > 0);

    if (env.IsCancelled())
      throw OperationCancelled{};

    p += nbytes;
  }
}

void
Port::FullWriteString(const char *s,
                      OperationEnvironment &env,
                      std::chrono::steady_clock::duration timeout)
{
  FullWrite(s, strlen(s), env, timeout);
}

std::byte
Port::ReadByte()
{
  std::byte b;
  if (Read(&b, sizeof(b)) != sizeof(b))
    throw std::runtime_error{"Port read failed"};

  return b;
}

void
Port::FullFlush(OperationEnvironment &env,
                std::chrono::steady_clock::duration timeout,
                std::chrono::steady_clock::duration _total_timeout)
{
  Flush();

  const TimeoutClock total_timeout(_total_timeout);

  do {
    try {
      WaitRead(env, timeout);
    } catch (const DeviceTimeout &) {
      return;
    }

    if (char buffer[0x100];
        Read(buffer, sizeof(buffer)) <= 0)
      throw std::runtime_error{"Port read failed"};
  } while (!total_timeout.HasExpired());
}

void
Port::FullRead(void *buffer, std::size_t length, OperationEnvironment &env,
               std::chrono::steady_clock::duration first_timeout,
               std::chrono::steady_clock::duration subsequent_timeout,
               std::chrono::steady_clock::duration total_timeout)
{
  const TimeoutClock full_timeout(total_timeout);

  char *p = (char *)buffer, *end = p + length;

  p += WaitAndRead(buffer, length, env, first_timeout);

  while (p < end) {
    const auto ft = full_timeout.GetRemainingSigned();
    if (ft.count() < 0)
      /* timeout */
      throw DeviceTimeout{"Port read timeout"};

    const auto t = std::min(ft, subsequent_timeout);

    p += WaitAndRead(p, end - p, env, t);
  }
}

void
Port::FullRead(void *buffer, std::size_t length, OperationEnvironment &env,
               std::chrono::steady_clock::duration timeout)
{
  FullRead(buffer, length, env, timeout, timeout, timeout);
}

void
Port::WaitRead(OperationEnvironment &env,
               std::chrono::steady_clock::duration timeout)
{
  auto remaining = timeout;

  do {
    /* this loop is ugly, and should be redesigned when we have
       non-blocking I/O in all Port implementations */
    const auto t = std::min<std::chrono::steady_clock::duration>(remaining, std::chrono::milliseconds(500));

    try {
      WaitRead(t);
      return;
    } catch (const DeviceTimeout &){
    }

    if (env.IsCancelled())
      throw OperationCancelled{};

    remaining -= t;
  } while (remaining.count() > 0);

  throw DeviceTimeout{"Port read timeout"};
}

std::size_t
Port::WaitAndRead(void *buffer, std::size_t length,
                  OperationEnvironment &env,
                  std::chrono::steady_clock::duration timeout)
{
  WaitRead(env, timeout);

  int nbytes = Read(buffer, length);
  if (nbytes <= 0)
    throw std::runtime_error{"Port read failed"};

  return (std::size_t)nbytes;
}

std::size_t
Port::WaitAndRead(void *buffer, std::size_t length,
                  OperationEnvironment &env, TimeoutClock timeout)
{
  const auto remaining = timeout.GetRemainingSigned();
  if (remaining.count() < 0)
    throw DeviceTimeout{"Port read timeout"};

  return WaitAndRead(buffer, length, env, remaining);
}

void
Port::ExpectString(const char *token, OperationEnvironment &env,
                   std::chrono::steady_clock::duration _timeout)
{
  const char *const token_end = token + strlen(token);

  const TimeoutClock timeout(_timeout);

  char buffer[256];

  const char *p = token;
  while (true) {
    auto nbytes = WaitAndRead(buffer,
                              std::min(sizeof(buffer),
                                       std::size_t(token_end - p)),
                              env, timeout);

    for (const char *q = buffer, *end = buffer + nbytes; q != end; ++q) {
      const char ch = *q;
      if (ch != *p)
        /* retry */
        p = token;
      else if (++p == token_end)
        return;
    }
  }
}

void
Port::WaitForChar(const char token, OperationEnvironment &env,
                  std::chrono::steady_clock::duration _timeout)
{
  const TimeoutClock timeout(_timeout);

  while (true) {
    WaitRead(env, timeout.GetRemainingOrZero());

    // Read and compare character with token
    const char ch = (char)ReadByte();
    if (ch == token)
      break;

    if (timeout.HasExpired())
      throw DeviceTimeout{"Port read timeout"};
  }
}

void
Port::StateChanged() noexcept
{
  PortListener *l = listener;
  if (l != nullptr)
    l->PortStateChanged();
}

void
Port::Error(const char *msg) noexcept
{
  PortListener *l = listener;
  if (l != nullptr)
    l->PortError(msg);
}

void
Port::Error(std::exception_ptr e) noexcept
{
  PortListener *l = listener;
  if (l != nullptr)
    l->PortError(GetFullMessage(e).c_str());
}
