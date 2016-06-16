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

#ifndef XCSOAR_TRACKING_SKYLINES_QUEUE_HPP
#define XCSOAR_TRACKING_SKYLINES_QUEUE_HPP

#include "Protocol.hpp"
#include "Util/OverwritingRingBuffer.hpp"

#include <stdint.h>

namespace SkyLinesTracking {

/**
 * This class stores FixPacket elements while the data connection is
 * offline, so we can post it as soon as we're back online.
 */
class Queue {
  /**
   * Don't queue any faster than this number of milliseconds.
   */
  static constexpr unsigned MIN_PERIOD_MS = 25000;

  OverwritingRingBuffer<FixPacket, 256> queue;

public:
  bool IsEmpty() const {
    return queue.empty();
  }

  void Push(const FixPacket &packet) {
    if (!IsEmpty() && (packet.time > queue.last().time &&
                       queue.last().time + MIN_PERIOD_MS < packet.time))
      return;

    queue.push(packet);
  }

  const FixPacket &Peek() {
    return queue.peek();
  }

  const FixPacket &Pop() {
    return queue.shift();
  }
};

} /* namespace SkyLinesTracking */

#endif
