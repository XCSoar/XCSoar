// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Tracking/SkyLines/Queue.hpp"
#include "util/ByteOrder.hxx"
#include "TestUtil.hpp"

using namespace SkyLinesTracking;

static FixPacket
MakePacket(uint32_t time_ms) noexcept
{
  FixPacket packet{};
  packet.time = ToBE32(time_ms);
  return packet;
}

static uint32_t
PopTime(Queue &queue) noexcept
{
  return FromBE32(queue.Pop().time);
}

int main()
{
  plan_tests(10);

  Queue queue;
  ok1(queue.IsEmpty());

  /* one fix every 5 seconds: keep one every 25 seconds */
  for (uint32_t t = 0; t <= 60000; t += 5000)
    queue.Push(MakePacket(36000000 + t));

  ok1(PopTime(queue) == 36000000);
  ok1(PopTime(queue) == 36025000);
  ok1(PopTime(queue) == 36050000);
  ok1(queue.IsEmpty());

  /* a gap longer than 25 seconds keeps the next fix */
  queue.Push(MakePacket(1000));
  queue.Push(MakePacket(100000));
  ok1(PopTime(queue) == 1000);
  ok1(PopTime(queue) == 100000);

  /* a time warp to an earlier time keeps the fix */
  queue.Push(MakePacket(500000));
  queue.Push(MakePacket(400000));
  ok1(PopTime(queue) == 500000);
  ok1(PopTime(queue) == 400000);
  ok1(queue.IsEmpty());

  return exit_status();
}
