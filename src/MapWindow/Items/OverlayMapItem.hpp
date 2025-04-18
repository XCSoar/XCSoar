// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapItem.hpp"

class MapOverlay;

/**
 * A #MapItem describing a #MapOverlay.
 */
struct OverlayMapItem : public MapItem
{
  const StaticString<64> label;

  explicit OverlayMapItem(const MapOverlay &_overlay);
};
