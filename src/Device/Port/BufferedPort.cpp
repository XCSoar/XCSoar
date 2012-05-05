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

#include "BufferedPort.hpp"
#include "TimeoutClock.hpp"

#include <algorithm>

#include <assert.h>

BufferedPort::BufferedPort(Port::Handler &_handler)
  :Port(_handler),
   running(false), waiting(false), closing(false)
{
}

#ifndef NDEBUG

BufferedPort::~BufferedPort()
{
  assert(closing);
  assert(!waiting);
}

#endif

void
BufferedPort::BeginClose()
{
  ScopeLock protect(mutex);
  closing = true;
#ifdef HAVE_POSIX
  cond.Signal();
#else
  data_trigger.Signal();
  consumed_trigger.Signal();
#endif
}

void
BufferedPort::EndClose()
{
  ScopeLock protect(mutex);
  /* make sure the callback isn't running */
  while (waiting) {
#ifdef HAVE_POSIX
    cond.Signal();
    cond.Wait(mutex);
#else
    data_trigger.Signal();
    mutex.Unlock();
    consumed_trigger.WaitAndReset();
    mutex.Lock();
#endif
  }
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

#ifdef HAVE_POSIX
  cond.Broadcast();
#else
  data_trigger.Signal();
  consumed_trigger.Signal();
#endif

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

#ifdef HAVE_POSIX
  cond.Broadcast();
#else
  data_trigger.Signal();
  consumed_trigger.Signal();
#endif

  return true;
}

int
BufferedPort::Read(void *dest, size_t length)
{
  assert(!closing);
  assert(!running);

  ScopeLock protect(mutex);

  auto r = buffer.Read();
  if (r.length == 0)
    return -1;

  size_t nbytes = std::min(length, r.length);
  std::copy(r.data, r.data + nbytes, (uint8_t *)dest);
  buffer.Consume(nbytes);

  if (waiting) {
    /* wake up the thread that may be waiting in DataReceived() */
#ifdef HAVE_POSIX
    cond.Broadcast();
#else
    consumed_trigger.Signal();
#endif
  }

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

#ifdef HAVE_POSIX
    cond.Wait(mutex, remaining_ms);
#else
    mutex.Unlock();
    data_trigger.Wait(remaining_ms);
    data_trigger.Reset();
    mutex.Lock();
#endif
  }

  return WaitResult::READY;
}

void
BufferedPort::DataReceived(const void *data, size_t length)
{
  assert(!waiting);

  if (running) {
    handler.DataReceived(data, length);
  } else {
    const uint8_t *p = (const uint8_t *)data;

    while (length > 0) {
      ScopeLock protect(mutex);

      auto r = buffer.Write();
      if (r.length > 0) {
        size_t nbytes = std::min(length, r.length);
        std::copy(p, p + nbytes, r.data);
        buffer.Append(nbytes);

        p += nbytes;
        length -= nbytes;

#ifdef HAVE_POSIX
        cond.Broadcast();
#else
        data_trigger.Signal();
#endif
      } else {
        if (closing)
          break;

        /* buffer is full; wait for another thread to consume data
           from it */
        waiting = true;
#ifdef HAVE_POSIX
        cond.Wait(mutex);
#else
        mutex.Unlock();
        consumed_trigger.WaitAndReset();
        mutex.Lock();
#endif
        waiting = false;

        if (closing) {
          /* while we were waiting, the destructor got called, and it
             is waiting for us to return  */
#ifdef HAVE_POSIX
          cond.Broadcast();
#else
          data_trigger.Signal();
#endif
          break;
        }

        if (!running)
          /* while we were waiting, somebody else has called
             StopRxThread() */
          break;
      }
    }
  }
}
