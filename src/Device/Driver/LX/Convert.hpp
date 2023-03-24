// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

class BufferedOutputStream;

namespace LX {
  /**
   * Convert a BLOB of LXN data to IGC, write to a file.
   */
  bool
  ConvertLXNToIGC(const void *data, size_t length,
                  BufferedOutputStream &os);
}
