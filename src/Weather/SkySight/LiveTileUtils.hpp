// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/custom/GeoBitmap.hpp"

namespace SkySight {

/**
 * Live images contain little additional visible detail at the geometrically
 * selected map zoom.  Use its immediate parent to reduce the request count,
 * except when the layer is already at its minimum zoom.
 */
[[nodiscard]] constexpr unsigned
SelectLiveTileZoom(unsigned map_zoom, unsigned minimum_zoom) noexcept
{
  return map_zoom > minimum_zoom ? map_zoom - 1 : minimum_zoom;
}

/**
 * The final live zoom is one below the geometric map zoom.  Let the geometric
 * selection advance one level beyond the provider maximum so that subtraction
 * can still reach the provider's highest supported tile level.
 */
[[nodiscard]] constexpr unsigned
GetLiveTileMapZoomMaximum(unsigned maximum_zoom) noexcept
{
  return maximum_zoom < GeoBitmap::MAX_TILE_ZOOM
    ? maximum_zoom + 1
    : maximum_zoom;
}

} // namespace SkySight
