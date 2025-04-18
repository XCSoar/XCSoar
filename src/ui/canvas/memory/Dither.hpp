// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/AllocatedArray.hxx"
#include "util/Compiler.h"

#include <cstdint>

class Dither {
  typedef int ErrorDistType; // must be wider than 8bits

  AllocatedArray<ErrorDistType> allocated_error_dist_buffer;

public:
  void DitherGreyscale(const uint8_t *gcc_restrict src,
                       unsigned src_pitch,
                       uint8_t *gcc_restrict dest,
                       unsigned dest_pitch,
                       unsigned width, unsigned height) noexcept;
};
