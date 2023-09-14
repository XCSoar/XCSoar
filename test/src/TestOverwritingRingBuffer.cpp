// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/OverwritingRingBuffer.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(17);

  OverwritingRingBuffer<unsigned,4> buffer;
  ok1(buffer.empty());

  buffer.push(1);
  ok1(!buffer.empty());
  ok1(buffer.peek() == 1);
  ok1(buffer.shift() == 1);
  ok1(buffer.empty());

  buffer.push(2);
  buffer.push(3);
  buffer.push(4);
  buffer.push(5);
  ok1(!buffer.empty());

  auto i = buffer.begin(), end = buffer.end();
  ok1(*i == 3);
  ++i;
  ok1(*i == 4);
  ++i;
  ok1(*i == 5);
  ++i;
  ok1(i == end);

  ok1(buffer.shift() == 3);
  ok1(buffer.shift() == 4);
  ok1(buffer.shift() == 5);
  ok1(buffer.empty());

  buffer.push(6);
  buffer.push(7);
  ok1(buffer.pop() == 7);
  ok1(buffer.pop() == 6);
  ok1(buffer.empty());

  return exit_status();
}
