// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Canvas;
class TwoTextRowsRenderer;
struct PixelRect;
struct Waypoint;
struct GeoVector;
struct WaypointLook;
struct WaypointRendererSettings;

namespace WaypointListRenderer
{
  void Draw(Canvas &canvas, const PixelRect rc, const Waypoint &waypoint,
            const TwoTextRowsRenderer &row_renderer,
            const WaypointLook &look,
            const WaypointRendererSettings &renderer_settings);

  void Draw(Canvas &canvas, const PixelRect rc, const Waypoint &waypoint,
            const GeoVector &vector,
            const TwoTextRowsRenderer &row_renderer,
            const WaypointLook &look,
            const WaypointRendererSettings &settings);

  void Draw(Canvas &canvas, const PixelRect rc, const Waypoint &waypoint,
            double distance, double arrival_altitude,
            const TwoTextRowsRenderer &row_renderer,
            const WaypointLook &look,
            const WaypointRendererSettings &settings);
}
