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
struct ComputerSettings;

/**
 * Renderer for the Turn Back Point (TBP)
 * Displays a green triangle on the track line at the point where
 * the aircraft would reach zero altitude difference to the task.
 */
class TurnBackPointRenderer {
  const MapLook &look;

public:
  constexpr TurnBackPointRenderer(const MapLook &_look) noexcept:look(_look) {}

  /**
   * Draw the TBP on the track line
   * @param canvas The canvas to draw on
   * @param projection The map projection
   * @param pos The aircraft position on screen
   * @param basic Basic NMEA info
   * @param calculated Calculated info including task stats
   * @param settings Computer settings
   */
  void Draw(Canvas &canvas,
            const WindowProjection &projection,
            const PixelPoint pos,
            const NMEAInfo &basic,
            const DerivedInfo &calculated,
            const ComputerSettings &settings) const noexcept;
};
