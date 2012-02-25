/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "PeriodClock.hpp"
#include "Operation/Operation.hpp"

#include <algorithm>
#include <assert.h>
#include <string.h>

Port::Port(Handler &_handler)
  :handler(_handler) {}

Port::~Port() {}

size_t
Port::Write(const char *s)
{
  return Write(s, strlen(s));
}

bool
Port::FullWrite(const void *buffer, size_t length, unsigned timeout_ms)
{
  PeriodClock timeout;
  timeout.Update();

  const char *p = (const char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.Check(timeout_ms))
      return false;

    size_t nbytes = Write(p, end - p);
    if (nbytes == 0)
      return false;

    p += nbytes;
  }

  return true;
}

bool
Port::FullWriteString(const char *s, unsigned timeout_ms)
{
  return FullWrite(s, strlen(s), timeout_ms);
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

  PeriodClock clock;
  clock.Update();

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
  } while (!clock.Check(total_timeout_ms));

  return true;
}

bool
Port::FullRead(void *buffer, size_t length, unsigned timeout_ms)
{
  PeriodClock timeout;
  timeout.Update();

  char *p = (char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.Check(timeout_ms))
      return false;

    int nbytes = Read(p, end - p);
    if (nbytes <= 0)
      return false;

    p += nbytes;
  }

  return true;
}

bool
Port::FullRead(void *buffer, size_t length, OperationEnvironment &env,
               unsigned timeout_ms)
{
  PeriodClock timeout;
  timeout.Update();

  unsigned remaining = timeout_ms;

  char *p = (char *)buffer, *end = p + length;
  while (p < end) {
    WaitResult wait_result = WaitRead(env, remaining);
    if (wait_result != WaitResult::READY)
      // Operation canceled, Timeout expired or I/O error occurred
      return false;

    int nbytes = Read(p, end - p);
    if (nbytes <= 0)
      /*
       * Error occured, or no data read, which is also an error
       * when WaitRead returns READY
       */
      return false;

    p += nbytes;

    unsigned elapsed = timeout.Elapsed();
    if (elapsed >= timeout_ms)
      return false;

    remaining = timeout_ms - elapsed;
  }

  return true;
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

bool
Port::ExpectString(const char *token, unsigned timeout_ms)
{
  assert(token != NULL);

  PeriodClock timeout;
  timeout.Update();

  const char *p = token;
  while (*p != '\0') {
    int ch = GetChar();
    if (ch == -1)
      return false;

    if (ch != *p++) {
      if (timeout.Check(timeout_ms))
        /* give up after 2 seconds (is that enough for all
           devices?) */
        return false;

      /* retry */
      p = token;
    }
  }

  return true;
}

Port::WaitResult
Port::WaitForChar(const char token, OperationEnvironment &env,
                  unsigned timeout_ms)
{
  PeriodClock timeout;
  timeout.Update();

  unsigned remaining = timeout_ms;

  while (true) {
    WaitResult wait_result = WaitRead(env, remaining);
    if (wait_result != WaitResult::READY)
      // Operation canceled, Timeout expired or I/O error occurred
      return wait_result;

    // Read and compare character with token
    int ch = GetChar();
    if (ch == token)
      break;

    unsigned elapsed = timeout.Elapsed();
    if (elapsed >= timeout_ms)
      return WaitResult::TIMEOUT;

    remaining = timeout_ms - elapsed;
  }

  return WaitResult::READY;
}
