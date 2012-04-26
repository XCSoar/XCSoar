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

#include "AndroidPort.hpp"
#include "Android/PortBridge.hpp"
#include "TimeoutClock.hpp"

#include <assert.h>

AndroidPort::AndroidPort(Port::Handler &_handler, PortBridge *_bridge)
  :Port(_handler), bridge(_bridge),
   running(false), waiting(false), closing(false)
{
  bridge->setListener(Java::GetEnv(), this);
}

AndroidPort::~AndroidPort()
{
  {
    ScopeLock protect(mutex);
    closing = true;
    cond.Signal();
  }

  delete bridge;

  {
    ScopeLock protect(mutex);
    /* make sure the callback isn't running */
    while (waiting) {
      cond.Signal();
      cond.Wait(mutex);
    }
  }
}

bool
AndroidPort::IsValid() const
{
  return bridge != NULL && bridge->isValid(Java::GetEnv());
}

void
AndroidPort::Flush()
{
  ScopeLock protect(mutex);
  buffer.Clear();
}

bool
AndroidPort::Drain()
{
  return bridge != NULL && bridge->drain(Java::GetEnv());
}

unsigned
AndroidPort::GetBaudrate() const
{
  return bridge != NULL
    ? bridge->getBaudRate(Java::GetEnv())
    : 0;
}

bool
AndroidPort::SetBaudrate(unsigned baud_rate)
{
  return bridge != NULL &&
    bridge->setBaudRate(Java::GetEnv(), baud_rate);
}

size_t
AndroidPort::Write(const void *data, size_t length)
{
  if (bridge == NULL)
    return 0;

  JNIEnv *env = Java::GetEnv();
  int nbytes = bridge->write(env, data, length);
  return nbytes > 0
    ? (size_t)nbytes
    : 0;
}

bool
AndroidPort::StopRxThread()
{
  ScopeLock protect(mutex);
  running = false;
  cond.Broadcast();
  return true;
}

bool
AndroidPort::StartRxThread()
{
  ScopeLock protect(mutex);
  if (!running) {
    running = true;
    buffer.Clear();
  }

  cond.Broadcast();

  return true;
}

int
AndroidPort::Read(void *dest, size_t length)
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
  return nbytes;
}

Port::WaitResult
AndroidPort::WaitRead(unsigned timeout_ms)
{
  TimeoutClock timeout(timeout_ms);
  ScopeLock protect(mutex);

  while (buffer.IsEmpty()) {
    if (running)
      return WaitResult::CANCELLED;

    int remaining_ms = timeout.GetRemainingSigned();
    if (remaining_ms <= 0)
      return WaitResult::TIMEOUT;

    cond.Wait(mutex, remaining_ms);
  }

  return WaitResult::READY;
}

void
AndroidPort::DataReceived(const void *data, size_t length)
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

        cond.Broadcast();
      } else {
        if (closing)
          break;

        waiting = true;
        cond.Wait(mutex);
        waiting = false;

        if (!running)
          /* while we were waiting, somebody else has called
             StopRxThread() */
          break;
      }
    }
  }
}
