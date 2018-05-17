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

#include "BufferedPort.hpp"
#include "Time/TimeoutClock.hpp"

#include <algorithm>

#include <assert.h>

BufferedPort::BufferedPort(PortListener *_listener, DataHandler &_handler)
  :Port(_listener, _handler),
   running(false), closing(false)
{
}

void
BufferedPort::BeginClose()
{
  ScopeLock protect(mutex);
  closing = true;
  cond.signal();
}

void
BufferedPort::EndClose()
{
}

void
BufferedPort::Flush()
{
  ScopeLock protect(mutex);
  buffer.Clear();
}

bool
BufferedPort::StopRxThread()
{
  ScopeLock protect(mutex);
  running = false;

  cond.broadcast();
  return true;
}

bool
BufferedPort::StartRxThread()
{
  ScopeLock protect(mutex);
  if (!running) {
    running = true;
    buffer.Clear();
  }

  cond.broadcast();
  return true;
}

int
BufferedPort::Read(void *dest, size_t length)
{
  assert(!closing);
  assert(!running);

  ScopeLock protect(mutex);

  auto r = buffer.Read();
  if (r.size == 0)
    return -1;

  size_t nbytes = std::min(length, r.size);
  std::copy_n(r.data, nbytes, (uint8_t *)dest);
  buffer.Consume(nbytes);
  return nbytes;
}

Port::WaitResult
BufferedPort::WaitRead(unsigned timeout_ms)
{
  TimeoutClock timeout(timeout_ms);
  ScopeLock protect(mutex);

  while (buffer.IsEmpty()) {
    if (running)
      return WaitResult::CANCELLED;

    int remaining_ms = timeout.GetRemainingSigned();
    if (remaining_ms <= 0)
      return WaitResult::TIMEOUT;

    cond.timed_wait(mutex, remaining_ms);
  }

  return WaitResult::READY;
}

void
BufferedPort::DataReceived(const void *data, size_t length)
{
  if (running) {
    handler.DataReceived(data, length);
  } else {
    const uint8_t *p = (const uint8_t *)data;

    ScopeLock protect(mutex);

    buffer.Shift();
    auto r = buffer.Write();
    if (r.size == 0)
      /* the buffer is already full, discard excess data */
      return;

    /* discard excess data */
    size_t nbytes = std::min(length, r.size);

    std::copy_n(p, nbytes, r.data);
    buffer.Append(nbytes);

    cond.broadcast();
  }
}
