// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class CoordinateFormat: uint8_t {
  DDMMSS = 0,
  DDMMSS_S,
  DDMM_MMM,
  DD_DDDDD,
  UTM,
};
