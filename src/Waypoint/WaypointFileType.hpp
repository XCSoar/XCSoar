// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

class Path;

enum class WaypointFileType: uint8_t {
  UNKNOWN,
  WINPILOT,
  SEEYOU,
  CUPX,
  ZANDER,
  OZI_EXPLORER,
  FS,
  COMPE_GPS,
};

[[gnu::pure]]
WaypointFileType
DetermineWaypointFileType(Path path) noexcept;
