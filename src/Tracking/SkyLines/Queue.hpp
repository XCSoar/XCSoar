// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Protocol.hpp"
#include "util/OverwritingRingBuffer.hpp"

#include <cstdint>

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
