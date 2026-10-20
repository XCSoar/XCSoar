// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * Where an airspace was loaded from.
 */
enum class AirspaceOrigin : uint8_t {
  /** Configured airspace files. */
  FILE,

  /** airspace.txt inside the map file. */
  MAP,

  /** Downloaded NOTAM areas. */
  NOTAM,
};
