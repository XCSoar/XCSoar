// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Protocol.hpp"
#include "util/ByteOrder.hxx"
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
    /* the times are big-endian; after a time warp to an earlier
       time, the unsigned difference is huge and the fix is kept */
    if (!IsEmpty() &&
        FromBE32(packet.time) - FromBE32(queue.last().time) < MIN_PERIOD_MS)
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
