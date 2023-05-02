// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BufferedPort.hpp"
#include "Device/Error.hpp"
#include "time/TimeoutClock.hpp"
#include "Operation/Cancelled.hpp"

#include <algorithm>

#include <cassert>

void
BufferedPort::Flush()
{
  const std::lock_guard lock{mutex};
  buffer.Clear();
}

bool
BufferedPort::StopRxThread()
{
  const std::lock_guard lock{mutex};
  running = false;

  cond.notify_all();
  return true;
}

bool
BufferedPort::StartRxThread()
{
  const std::lock_guard lock{mutex};
  if (!running) {
    running = true;
    buffer.Clear();
  }

  cond.notify_all();
  return true;
}

std::size_t
BufferedPort::Read(std::span<std::byte> dest)
{
  assert(!running);

  const std::lock_guard lock{mutex};

  auto r = buffer.Read();
  if (r.size() > dest.size())
    r = r.first(dest.size());
  std::copy(r.begin(), r.end(), dest.data());
  buffer.Consume(r.size());
  return r.size();
}

void
BufferedPort::WaitRead(std::chrono::steady_clock::duration _timeout)
{
  TimeoutClock timeout(_timeout);
  std::unique_lock lock{mutex};

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
    const std::lock_guard lock{mutex};

    buffer.Shift();
    auto r = buffer.Write();
    if (r.empty())
      /* the buffer is already full, discard excess data */
      return true;

    /* discard excess data */
    const std::size_t nbytes = std::min(s.size(), r.size());

    std::copy_n(s.begin(), nbytes, r.begin());
    buffer.Append(nbytes);

    cond.notify_all();
    return true;
  }
}
