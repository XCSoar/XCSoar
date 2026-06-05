// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "Look/MapLook.hpp"
#include "Renderer/DistanceRingsRenderer.hpp"

void
MapWindow::DrawDistanceRings(Canvas &canvas) const noexcept
{
  if (!GetMapSettings().distance_rings_enabled)
    return;

  const auto &basic = Basic();
  if (!basic.location_available)
    return;

  ::DrawDistanceRings(canvas, render_projection, basic.location, look);
}
