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

#include "BufferedPort.hpp"
#include "Device/Error.hpp"
#include "time/TimeoutClock.hpp"
#include "Operation/Cancelled.hpp"

#include <algorithm>

#include <cassert>

void
BufferedPort::Flush()
{
  std::lock_guard<Mutex> lock(mutex);
  buffer.Clear();
}

bool
BufferedPort::StopRxThread()
{
  std::lock_guard<Mutex> lock(mutex);
  running = false;

  cond.notify_all();
  return true;
}

bool
BufferedPort::StartRxThread()
{
  std::lock_guard<Mutex> lock(mutex);
  if (!running) {
    running = true;
    buffer.Clear();
  }

  cond.notify_all();
  return true;
}

std::size_t
BufferedPort::Read(void *dest, std::size_t length)
{
  assert(!running);

  std::lock_guard<Mutex> lock(mutex);

  auto r = buffer.Read();
  std::size_t nbytes = std::min(length, r.size);
  std::copy_n(r.data, nbytes, (std::byte *)dest);
  buffer.Consume(nbytes);
  return nbytes;
}

void
BufferedPort::WaitRead(std::chrono::steady_clock::duration _timeout)
{
  TimeoutClock timeout(_timeout);
  std::unique_lock<Mutex> lock(mutex);

  while (buffer.empty()) {
    if (running)
      throw OperationCancelled{};

    auto remaining = timeout.GetRemainingSigned();
    if (remaining.count() <= 0)
      throw DeviceTimeout{"Timeout"};

    cond.wait_for(lock, remaining);
  }
}

bool
BufferedPort::DataReceived(std::span<const std::byte> s) noexcept
{
  if (running) {
    return handler.DataReceived(s);
  } else {
    std::lock_guard<Mutex> lock(mutex);

    buffer.Shift();
    auto r = buffer.Write();
    if (r.size == 0)
      /* the buffer is already full, discard excess data */
      return true;

    /* discard excess data */
    const std::size_t nbytes = std::min(s.size(), r.size);

    std::copy_n(s.data(), nbytes, r.data);
    buffer.Append(nbytes);

    cond.notify_all();
    return true;
  }
}
