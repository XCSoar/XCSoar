// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelPoint;
class Canvas;
class Angle;
class WindowProjection;
struct MapLook;
struct NMEAInfo;
struct DerivedInfo;
struct MapSettings;

/**
 * Renderer for the track line
 * Renders line forward along the current ground track (not heading) in straight flight
 * or curve using current rate of curve when curving / circling.
 */
class TrackLineRenderer {
  const MapLook &look;

public:
  TrackLineRenderer(const MapLook &_look):look(_look) {}

  void Draw(Canvas &canvas, const Angle screen_angle, const Angle track_angle,
            PixelPoint pos);

  void Draw(Canvas &canvas,
            const WindowProjection &projection,
            PixelPoint pos, const NMEAInfo &basic,
            const DerivedInfo &calculated, const MapSettings &settings,
            bool wind_relative);

protected:
  void DrawProjected(Canvas &canvas,
                     const WindowProjection &projection,
                     const NMEAInfo &basic,
                     const DerivedInfo &calculated,
                     const MapSettings &settings,
                     bool wind_relative);
};
