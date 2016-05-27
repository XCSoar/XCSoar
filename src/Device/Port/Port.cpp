/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Time/TimeoutClock.hpp"
#include "Operation/Operation.hpp"

#include <algorithm>
#include <string.h>

Port::Port(PortListener *_listener, DataHandler &_handler)
  :listener(_listener), handler(_handler) {}

Port::~Port() {}

bool
Port::WaitConnected(OperationEnvironment &env)
{
  while (GetState() == PortState::LIMBO && !env.IsCancelled())
    env.Sleep(200);

  return GetState() == PortState::READY;
}

size_t
Port::Write(const char *s)
{
  return Write(s, strlen(s));
}

bool
Port::FullWrite(const void *buffer, size_t length,
                OperationEnvironment &env, unsigned timeout_ms)
{
  const TimeoutClock timeout(timeout_ms);

  const char *p = (const char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.HasExpired())
      return false;

    size_t nbytes = Write(p, end - p);
    if (nbytes == 0 || env.IsCancelled())
      return false;

    p += nbytes;
  }

  return true;
}

bool
Port::FullWriteString(const char *s,
                      OperationEnvironment &env, unsigned timeout_ms)
{
  return FullWrite(s, strlen(s), env, timeout_ms);
}

int
Port::GetChar()
{
  unsigned char ch;
  return Read(&ch, sizeof(ch)) == sizeof(ch)
    ? ch
    : -1;
}

bool
Port::FullFlush(OperationEnvironment &env, unsigned timeout_ms,
                unsigned total_timeout_ms)
{
  Flush();

  const TimeoutClock total_timeout(total_timeout_ms);

  char buffer[0x100];
  do {
    switch (WaitRead(env, timeout_ms)) {
    case WaitResult::READY:
      if (!Read(buffer, sizeof(buffer)))
        return false;
      break;

    case WaitResult::TIMEOUT:
      return true;

    case WaitResult::FAILED:
    case WaitResult::CANCELLED:
      return false;
    }
  } while (!total_timeout.HasExpired());

  return true;
}

bool
Port::FullRead(void *buffer, size_t length, OperationEnvironment &env,
               unsigned first_timeout_ms, unsigned subsequent_timeout_ms,
               unsigned total_timeout_ms)
{
  const TimeoutClock full_timeout(total_timeout_ms);

  char *p = (char *)buffer, *end = p + length;

  size_t nbytes = WaitAndRead(buffer, length, env, first_timeout_ms);
  if (nbytes <= 0)
    return false;

  p += nbytes;

  while (p < end) {
    const int ft = full_timeout.GetRemainingSigned();
    if (ft < 0)
      /* timeout */
      return false;

    const unsigned t = std::min(unsigned(ft), subsequent_timeout_ms);

    nbytes = WaitAndRead(p, end - p, env, t);
    if (nbytes == 0)
      /*
       * Error occured, or no data read, which is also an error
       * when WaitRead returns READY
       */
      return false;

    p += nbytes;
  }

  return true;
}

bool
Port::FullRead(void *buffer, size_t length, OperationEnvironment &env,
               unsigned timeout_ms)
{
  return FullRead(buffer, length, env, timeout_ms, timeout_ms, timeout_ms);
}

Port::WaitResult
Port::WaitRead(OperationEnvironment &env, unsigned timeout_ms)
{
  unsigned remaining = timeout_ms;

  do {
    /* this loop is ugly, and should be redesigned when we have
       non-blocking I/O in all Port implementations */
    const unsigned t = std::min(remaining, 500u);
    WaitResult result = WaitRead(t);
    if (result != WaitResult::TIMEOUT)
      return result;

    if (env.IsCancelled())
      return WaitResult::CANCELLED;

    remaining -= t;
  } while (remaining > 0);

  return WaitResult::TIMEOUT;
}

size_t
Port::WaitAndRead(void *buffer, size_t length,
                  OperationEnvironment &env, unsigned timeout_ms)
{
  WaitResult wait_result = WaitRead(env, timeout_ms);
  if (wait_result != WaitResult::READY)
    // Operation canceled, Timeout expired or I/O error occurred
    return 0;

  int nbytes = Read(buffer, length);
  if (nbytes < 0)
    return 0;

  return (size_t)nbytes;
}

size_t
Port::WaitAndRead(void *buffer, size_t length,
                  OperationEnvironment &env, TimeoutClock timeout)
{
  int remaining = timeout.GetRemainingSigned();
  if (remaining < 0)
    return 0;

  return WaitAndRead(buffer, length, env, remaining);
}

bool
Port::ExpectString(const char *token, OperationEnvironment &env,
                   unsigned timeout_ms)
{
  const char *const token_end = token + strlen(token);

  const TimeoutClock timeout(timeout_ms);

  char buffer[256];

  const char *p = token;
  while (true) {
    size_t nbytes = WaitAndRead(buffer,
                                std::min(sizeof(buffer), size_t(token_end - p)),
                                env, timeout);
    if (nbytes == 0 || env.IsCancelled())
      return false;

    for (const char *q = buffer, *end = buffer + nbytes; q != end; ++q) {
      const char ch = *q;
      if (ch != *p)
        /* retry */
        p = token;
      else if (++p == token_end)
        return true;
    }
  }
}

Port::WaitResult
Port::WaitForChar(const char token, OperationEnvironment &env,
                  unsigned timeout_ms)
{
  const TimeoutClock timeout(timeout_ms);

  while (true) {
    WaitResult wait_result = WaitRead(env, timeout.GetRemainingOrZero());
    if (wait_result != WaitResult::READY)
      // Operation canceled, Timeout expired or I/O error occurred
      return wait_result;

    // Read and compare character with token
    int ch = GetChar();
    if (ch == token)
      break;

    if (timeout.HasExpired())
      return WaitResult::TIMEOUT;
  }

  return WaitResult::READY;
}

void
Port::StateChanged()
{
  PortListener *l = listener;
  if (l != nullptr)
    l->PortStateChanged();
}

void
Port::Error(const char *msg)
{
  PortListener *l = listener;
  if (l != nullptr)
    l->PortError(msg);
}
