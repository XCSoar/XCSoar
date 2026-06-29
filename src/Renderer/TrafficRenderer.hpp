// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Color.hpp"

struct PixelPoint;
class Canvas;
struct TrafficLook;
struct FlarmTraffic;
struct GliderLinkTraffic;
class Angle;

namespace TrafficRenderer
{
void
Draw(Canvas &canvas, const TrafficLook &traffic_look,
     bool fading,
     const FlarmTraffic &traffic, Angle angle,
     FlarmColor color, PixelPoint pt) noexcept;

/**
 * Draw a traffic symbol scaled to fit a list row icon slot.
 */
void
DrawList(Canvas &canvas, const TrafficLook &traffic_look,
         const FlarmTraffic &traffic, Angle angle,
         FlarmColor color, PixelPoint pt,
         unsigned icon_size) noexcept;

void
Draw(Canvas &canvas, const TrafficLook &traffic_look,
     const GliderLinkTraffic &traffic, Angle angle, PixelPoint pt) noexcept;

/**
 * Pixel height of map traffic symbols (DPI-aware, not window size).
 */
[[gnu::const]]
unsigned MapIconSize() noexcept;

/**
 * Label offsets for map traffic symbols, derived from #MapIconSize().
 */
struct MapTrafficLabelLayout {
  unsigned icon_size;
  /** Subtract from symbol centre Y for the callsign anchor. */
  int name_offset_y;
  /** Add to symbol centre Y for the climb-rate anchor. */
  int climb_offset_y;
  /** Minimum own-ship distance (px) before labels are drawn. */
  int min_label_distance;
};

[[gnu::const]]
MapTrafficLabelLayout MapLabelLayout() noexcept;
}
