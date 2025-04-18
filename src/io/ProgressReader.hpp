// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Reader.hxx"
#include "Operation/ProgressListener.hpp"

#include <cstdint>

/**
 * A #Reader facade that reports progress to a #ProgressListener.
 */
class ProgressReader final : public Reader {
  Reader &next;

  ProgressListener &progress_listener;

  const uint_least64_t total;
  uint_least64_t position = 0;

public:
  /**
   * @param _total the total size of all data expected to be read from
   * #_reader
   */
  ProgressReader(Reader &_next, uint_least64_t _total,
                 ProgressListener &_progress_listener) noexcept
    :next(_next), progress_listener(_progress_listener), total(_total) {
    progress_listener.SetProgressRange(1024);
  }

  std::size_t Read(std::span<std::byte> dest) override {
    std::size_t nbytes = next.Read(dest);

    position += nbytes;

    // TODO rate-limit SetProgressRange() calls?
    if (nbytes > 0 && total > 0)
      progress_listener.SetProgressPosition(1024 * position / total);

    return nbytes;
  }
};
