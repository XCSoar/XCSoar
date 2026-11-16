// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <cstdint>

void FormatByteSize(char *buffer, size_t size,
                    uint64_t bytes, bool simple = false);

/**
 * "12.0 MB/48.0 MB" when the bar counts bytes, otherwise "12000/50000".
 * A percent bar (0..100) stays numeric even if a byte rate is shown.
 */
void FormatProgressAmount(char *buffer, size_t size,
                          unsigned done, unsigned total,
                          bool bytes, unsigned byte_count) noexcept;
