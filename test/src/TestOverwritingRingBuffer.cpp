/* Copyright_License {

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

#include "Util/OverwritingRingBuffer.hpp"
#include "TestUtil.hpp"

int main(int argc, char **argv)
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
