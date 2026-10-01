// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Color.hpp"
#include "FLARM/Traffic.hpp"

#include <cstdint>

struct PixelPoint;
class Canvas;
class Color;
class Pen;
struct TrafficLook;
struct GliderLinkTraffic;
class Angle;
enum class TrafficSymbol : uint8_t;

namespace TrafficRenderer
{
/**
 * Draw a FLARM traffic symbol on the map.
 *
 * @param symbol the symbol style from #MapSettings::traffic_symbol
 */
void
Draw(Canvas &canvas, const TrafficLook &traffic_look,
     TrafficSymbol symbol, bool fading,
     const FlarmTraffic &traffic, Angle angle,
     FlarmColor color, PixelPoint pt) noexcept;

/**
 * Draw a traffic symbol scaled to fit a list row icon slot.
 */
void
DrawList(Canvas &canvas, const TrafficLook &traffic_look,
         TrafficSymbol symbol,
         const FlarmTraffic &traffic, Angle angle,
         FlarmColor color, PixelPoint pt,
         unsigned icon_size) noexcept;

void
Draw(Canvas &canvas, const TrafficLook &traffic_look,
     TrafficSymbol symbol,
     const GliderLinkTraffic &traffic, Angle angle, PixelPoint pt) noexcept;

/**
 * Is there an aircraft-type symbol for this type?  A type without one
 * is drawn as the classic arrow head.
 */
[[gnu::const]]
bool
HasAircraftTypeSymbol(FlarmTraffic::AircraftType type) noexcept;

/**
 * How the aircraft-type symbols are composed from the icon's halo
 * silhouette and its glyph.
 */
enum class AircraftTypeSymbolStyle : uint8_t {
  /**
   * The halo is filled with the traffic colour without any border;
   * the glyph on top of it is drawn in the glyph colour.  Looks like
   * the icon with its white replaced by the traffic colour, the way
   * the tinting shader of XCSoar PR #2538 renders the bitmaps.
   */
  COLOURED_HALO,

  /**
   * Like COLOURED_HALO, plus the border pen around the halo.
   */
  COLOURED_HALO_OUTLINED,

  /**
   * The halo keeps the icon's white (or the background colour) and
   * has no border; the glyph on top of it carries the traffic colour.
   * Looks like the icon with its black replaced by the traffic colour.
   */
  WHITE_HALO,

  /**
   * No halo; the glyph is filled with the traffic colour and outlined
   * with a hairline in the glyph colour, like the classic arrow head.
   */
  BLACK_OUTLINE,
};

/**
 * The style used for all aircraft-type symbols.
 */
static constexpr AircraftTypeSymbolStyle AIRCRAFT_TYPE_SYMBOL_STYLE =
  AircraftTypeSymbolStyle::COLOURED_HALO;

/**
 * Draw the aircraft-type symbol according to #AIRCRAFT_TYPE_SYMBOL_STYLE.
 * Works on all canvas backends because the symbol is drawn as rotated
 * polygons.
 *
 * @param size the height of the (square) symbol box in pixels; the
 * glyph itself is somewhat smaller
 * @param body_color the traffic colour (altitude / alarm)
 * @param glyph_color the contrast colour of the glyph (COLOURED_HALO
 * styles) or of its outline (BLACK_OUTLINE)
 * @param halo_color the halo colour, usually the background colour
 * (WHITE_HALO)
 * @param border_pen the pen for the outline of the halo, e.g. the
 * pen that would draw the classic arrow head (COLOURED_HALO_OUTLINED)
 */
void
DrawAircraftTypeSymbol(Canvas &canvas, FlarmTraffic::AircraftType type,
                       Angle angle, PixelPoint pt, unsigned size,
                       Color body_color, Color glyph_color, Color halo_color,
                       const Pen &border_pen) noexcept;

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
