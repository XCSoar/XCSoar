// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"

void
MapWindow::DrawWaypoints(Canvas &canvas) noexcept
{
  waypoint_renderer.Render(canvas, label_block,
                           render_projection, GetMapSettings().waypoint,
                           GetComputerSettings().polar,
                           GetComputerSettings().task,
                           Basic(), Calculated(),
                           task, route_planner);
}
