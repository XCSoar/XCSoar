// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapWindow/MbTilesOverlay.hpp"

/**
 * EDL-specific MBTiles overlay with What's-here value lookup.
 */
class EdlMbTilesOverlay final : public MbTilesOverlay {
public:
  EdlMbTilesOverlay(Path path, std::string label);

  bool
  SampleAscendancyAt(GeoPoint p, double &value_mps) const noexcept;
};
