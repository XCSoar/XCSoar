// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapItem.hpp"
#include "Geo/GeoPoint.hpp"

class MapOverlay;

/**
 * A #MapItem describing a #MapOverlay.
 */
struct OverlayMapItem : public MapItem
{
  const StaticString<64> label;

  /**
   * Optional second line with the overlay's value at the tapped
   * location (e.g. the XCTherm forecast climb / altitude / download
   * time). Empty when the overlay provides no point detail.
   */
  StaticString<256> detail;

  OverlayMapItem(const MapOverlay &_overlay, GeoPoint location);
};
