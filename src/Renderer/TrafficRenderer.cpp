// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficRenderer.hpp"
#include "AircraftTypeSymbolData.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/TrafficLook.hpp"
#include "FLARM/Traffic.hpp"
#include "GliderLink/Traffic.hpp"
#include "MapSettings.hpp"
#include "Math/Screen.hpp"
#include "Math/FastRotation.hpp"
#include "Math/Util.hpp"
#include "util/Macros.hpp"
#include "Asset.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <span>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

/** The arrow template spans 14 units vertically (-8 to +6). */
static constexpr unsigned ARROW_SPAN = 14;

/**
 * Target map traffic symbol height in virtual points.  Scales with
 * display DPI (and small-screen viewing distance) but not window size.
 * ~Scale(100) arrow size at the 240 px design baseline.
 */
static constexpr unsigned MAP_TRAFFIC_ICON_VPT = 50;

/**
 * The aircraft-type symbol box occupies this fraction of the icon
 * slot; the glyph inside the box is a bit smaller still, which keeps
 * the team circle and the labels clear of it.
 */
static constexpr unsigned SYMBOL_BOX_NUMERATOR = 7;
static constexpr unsigned SYMBOL_BOX_DENOMINATOR = 10;

/**
 * Radius of the team circle around an aircraft-type symbol as a
 * fraction of the icon slot; large enough to enclose the widest glyph
 * at any rotation, small enough to stay clear of the labels.
 */
static constexpr unsigned SYMBOL_CIRCLE_NUMERATOR = 9;
static constexpr unsigned SYMBOL_CIRCLE_DENOMINATOR = 20;

struct MapTrafficScale {
  int arrow_scale;
  unsigned circle_radius;
  unsigned symbol_size;
  unsigned symbol_circle_radius;
};

[[gnu::pure]]
static MapTrafficScale
MapTrafficScaleFromIconSize(unsigned icon_size) noexcept
{
  return {
    std::max(int(icon_size) * 50 / int(ARROW_SPAN), 1),
    std::max(icon_size / 3U, Layout::ScalePenWidth(1)),
    std::max(icon_size * SYMBOL_BOX_NUMERATOR / SYMBOL_BOX_DENOMINATOR, 1U),
    std::max(icon_size * SYMBOL_CIRCLE_NUMERATOR / SYMBOL_CIRCLE_DENOMINATOR,
             Layout::ScalePenWidth(1)),
  };
}

[[gnu::pure]]
static MapTrafficScale
GetMapTrafficScale() noexcept
{
  return MapTrafficScaleFromIconSize(Layout::VptScale(MAP_TRAFFIC_ICON_VPT));
}

[[gnu::const]]
static const AircraftTypeSymbol &
GetAircraftTypeSymbol(FlarmTraffic::AircraftType type) noexcept
{
  const unsigned index = unsigned(type);
  return aircraft_type_symbols[index < ARRAY_SIZE(aircraft_type_symbols)
                               ? index : 0];
}

bool
TrafficRenderer::HasAircraftTypeSymbol(FlarmTraffic::AircraftType type) noexcept
{
  return GetAircraftTypeSymbol(type).IsDefined();
}

[[gnu::pure]]
static bool
UseAircraftTypeSymbol(TrafficSymbol symbol,
                      FlarmTraffic::AircraftType type) noexcept
{
  return symbol == TrafficSymbol::AIRCRAFT_TYPE &&
    TrafficRenderer::HasAircraftTypeSymbol(type);
}

/**
 * Draw polygons of a symbol with the currently selected pen and
 * brush.
 *
 * @param size the template box maps to this many pixels
 * @param holes draw the holes instead of the solid parts
 *
 * PolygonRotateShift() is not used here because it divides the scale
 * by 25 in integer arithmetic, which is far too coarse for symbol
 * sizes of a few dozen pixels; the points are rotated and scaled in
 * floating point and rounded once.
 */
static void
DrawSymbolShape(Canvas &canvas,
                std::span<const AircraftTypeSymbolPolygon> polygons,
                const Angle angle, const PixelPoint pt, unsigned size,
                bool holes) noexcept
{
  const FastRotation rotation(angle);
  const double scale = double(size) / AIRCRAFT_TYPE_SYMBOL_SPAN;

  for (const auto &polygon : polygons) {
    if (polygon.hole != holes)
      continue;

    std::array<BulkPixelPoint, MAX_AIRCRAFT_TYPE_SYMBOL_POINTS> buffer;
    assert(polygon.points.size() <= buffer.size());

    const std::size_t n = std::min(polygon.points.size(), buffer.size());
    if (n < 3)
      continue;

    for (std::size_t i = 0; i < n; ++i) {
      const auto r = rotation.Rotate(DoublePoint2D{
          polygon.points[i].x * scale,
          polygon.points[i].y * scale,
        });
      buffer[i] = BulkPixelPoint(pt.x + iround(r.x), pt.y + iround(r.y));
    }

    canvas.DrawPolygon(buffer.data(), n);
  }
}

void
TrafficRenderer::DrawAircraftTypeSymbol(Canvas &canvas,
                                        FlarmTraffic::AircraftType type,
                                        const Angle angle, const PixelPoint pt,
                                        const unsigned size,
                                        const Color body_color,
                                        const Color glyph_color,
                                        const Color halo_color,
                                        const Pen &border_pen) noexcept
{
  const AircraftTypeSymbol &symbol = GetAircraftTypeSymbol(type);
  if (!symbol.IsDefined() || size == 0)
    return;

  /* the brushes must outlive the drawing calls (GDI selects the
     native object into the device context) */
  const Brush body_brush(body_color);

  if (AIRCRAFT_TYPE_SYMBOL_STYLE == AircraftTypeSymbolStyle::COLOURED_HALO ||
      AIRCRAFT_TYPE_SYMBOL_STYLE == AircraftTypeSymbolStyle::COLOURED_HALO_OUTLINED) {
    const Brush glyph_brush(glyph_color);

    /* the halo silhouette carries the traffic colour, optionally with
       the same border the classic arrow head would have */
    if (AIRCRAFT_TYPE_SYMBOL_STYLE == AircraftTypeSymbolStyle::COLOURED_HALO)
      canvas.SelectNullPen();
    else
      canvas.Select(border_pen);
    canvas.Select(body_brush);
    DrawSymbolShape(canvas, symbol.halo, angle, pt, size, false);

    /* the glyph itself is drawn on top in the glyph colour, exactly
       as designed in the icon; its holes and highlights show the
       traffic colour again */
    canvas.SelectNullPen();
    canvas.Select(glyph_brush);
    DrawSymbolShape(canvas, symbol.glyph, angle, pt, size, false);
    canvas.Select(body_brush);
    DrawSymbolShape(canvas, symbol.glyph, angle, pt, size, true);
  } else if (AIRCRAFT_TYPE_SYMBOL_STYLE == AircraftTypeSymbolStyle::WHITE_HALO) {
    const Brush halo_brush(halo_color);

    /* the halo keeps the icon's white and separates the glyph from
       the map; no border line */
    canvas.SelectNullPen();
    canvas.Select(halo_brush);
    DrawSymbolShape(canvas, symbol.halo, angle, pt, size, false);

    /* the glyph carries the traffic colour; holes and highlights show
       the halo again */
    canvas.Select(body_brush);
    DrawSymbolShape(canvas, symbol.glyph, angle, pt, size, false);
    canvas.Select(halo_brush);
    DrawSymbolShape(canvas, symbol.glyph, angle, pt, size, true);
  } else {
    /* no halo: the glyph is filled with the traffic colour and gets a
       hairline outline, like the classic arrow head; holes and
       highlights are outlined as well, which keeps the structure */
    const Pen outline_pen(Layout::ScalePenWidth(1), glyph_color);

    canvas.Select(outline_pen);
    canvas.Select(body_brush);
    DrawSymbolShape(canvas, symbol.glyph, angle, pt, size, false);
    DrawSymbolShape(canvas, symbol.glyph, angle, pt, size, true);
  }

  /* deselect the temporaries before they are destroyed */
  canvas.SelectBlackPen();
  canvas.SelectHollowBrush();
}

/**
 * Draw the fading variant of the aircraft-type symbol: a dashed
 * outline, translucent on OpenGL.
 */
static void
DrawFadingAircraftTypeSymbol(Canvas &canvas, const TrafficLook &traffic_look,
                             FlarmTraffic::AircraftType type,
                             const Angle angle, const PixelPoint pt,
                             const unsigned size) noexcept
{
  canvas.Select(traffic_look.fading_pen);

#ifdef ENABLE_OPENGL
  canvas.Select(traffic_look.fading_brush);
  const ScopeAlphaBlend alpha_blend;
#else
  /* we have no alpha blending - don't fill the shape */
  canvas.SelectHollowBrush();
#endif

  DrawSymbolShape(canvas, GetAircraftTypeSymbol(type).halo, angle, pt, size,
                  false);
}

static void
DrawTeamCircle(Canvas &canvas, const TrafficLook &traffic_look,
               const FlarmColor color, const PixelPoint pt,
               unsigned circle_radius) noexcept
{
  switch (color) {
  case FlarmColor::GREEN:
    canvas.Select(traffic_look.team_pen_green);
    break;
  case FlarmColor::BLUE:
    canvas.Select(traffic_look.team_pen_blue);
    break;
  case FlarmColor::YELLOW:
    canvas.Select(traffic_look.team_pen_yellow);
    break;
  case FlarmColor::MAGENTA:
    canvas.Select(traffic_look.team_pen_magenta);
    break;
  default:
    return;
  }

  canvas.SelectHollowBrush();
  canvas.DrawCircle(pt, circle_radius);
}

static void
DrawFlarmArrow(Canvas &canvas, const TrafficLook &traffic_look,
               bool fading, const FlarmTraffic &traffic,
               const Angle angle, const PixelPoint pt,
               int arrow_scale) noexcept
{
  BulkPixelPoint arrow[] = {
    { -4, 6 },
    { 0, -8 },
    { 4, 6 },
    { 0, 3 },
  };

  PolygonRotateShift(arrow, pt, angle, arrow_scale);

  if (fading) {
    canvas.Select(traffic_look.fading_pen);

#ifdef ENABLE_OPENGL
    canvas.Select(traffic_look.fading_brush);
#else
    /* we have no alpha blending - don't fill the shape */
    canvas.SelectHollowBrush();
#endif

#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));
  } else {
    canvas.Select(traffic_look.GetBodyBrush(traffic));
    canvas.SelectBlackPen();
    canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));
  }
}

static void
DrawFlarmSymbol(Canvas &canvas, const TrafficLook &traffic_look,
                TrafficSymbol symbol, bool fading,
                const FlarmTraffic &traffic,
                const Angle angle, const FlarmColor color,
                const PixelPoint pt,
                const MapTrafficScale &scale) noexcept
{
  if (UseAircraftTypeSymbol(symbol, traffic.type)) {
    if (fading)
      DrawFadingAircraftTypeSymbol(canvas, traffic_look, traffic.type,
                                   angle, pt, scale.symbol_size);
    else
      TrafficRenderer::DrawAircraftTypeSymbol(canvas, traffic.type,
                                              angle, pt, scale.symbol_size,
                                              traffic_look.GetBodyColor(traffic),
                                              traffic_look.symbol_glyph_color,
                                              traffic_look.symbol_halo_color,
                                              traffic_look.symbol_border_pen);

    DrawTeamCircle(canvas, traffic_look, color, pt,
                   scale.symbol_circle_radius);
  } else {
    DrawFlarmArrow(canvas, traffic_look, fading, traffic, angle, pt,
                   scale.arrow_scale);
    DrawTeamCircle(canvas, traffic_look, color, pt, scale.circle_radius);
  }
}

unsigned
TrafficRenderer::MapIconSize() noexcept
{
  return Layout::VptScale(MAP_TRAFFIC_ICON_VPT);
}

TrafficRenderer::MapTrafficLabelLayout
TrafficRenderer::MapLabelLayout() noexcept
{
  const unsigned icon_size = MapIconSize();
  const int half = int(icon_size) / 2;

  return {
    icon_size,
    half + int(Layout::VptScale(3)),
    half + int(Layout::VptScale(1)),
    half + int(Layout::VptScale(30)),
  };
}

void
TrafficRenderer::Draw(Canvas &canvas, const TrafficLook &traffic_look,
                      TrafficSymbol symbol, bool fading,
                      const FlarmTraffic &traffic, const Angle angle,
                      const FlarmColor color, const PixelPoint pt) noexcept
{
  DrawFlarmSymbol(canvas, traffic_look, symbol, fading, traffic, angle,
                  color, pt, GetMapTrafficScale());
}

void
TrafficRenderer::DrawList(Canvas &canvas, const TrafficLook &traffic_look,
                          TrafficSymbol symbol,
                          const FlarmTraffic &traffic, const Angle angle,
                          const FlarmColor color, const PixelPoint pt,
                          unsigned icon_size) noexcept
{
  DrawFlarmSymbol(canvas, traffic_look, symbol, false, traffic, angle,
                  color, pt, MapTrafficScaleFromIconSize(icon_size));
}

void
TrafficRenderer::Draw(Canvas &canvas, const TrafficLook &traffic_look,
                      TrafficSymbol symbol,
                      [[maybe_unused]] const GliderLinkTraffic &traffic,
                      const Angle angle, const PixelPoint pt) noexcept
{
  const MapTrafficScale scale = GetMapTrafficScale();

  if (UseAircraftTypeSymbol(symbol, FlarmTraffic::AircraftType::GLIDER)) {
    /* GliderLink carries no aircraft type; everything on that
       network is a glider */
    DrawAircraftTypeSymbol(canvas, FlarmTraffic::AircraftType::GLIDER,
                           angle, pt, scale.symbol_size,
                           traffic_look.safe_above_color,
                           traffic_look.symbol_glyph_color,
                           traffic_look.symbol_halo_color,
                           traffic_look.symbol_border_pen);

    canvas.SelectHollowBrush();
    canvas.DrawCircle(pt, scale.symbol_circle_radius);
    return;
  }

  BulkPixelPoint arrow[] = {
    { -4, 6 },
    { 0, -8 },
    { 4, 6 },
    { 0, 3 },
  };

  canvas.Select(traffic_look.safe_above_brush);

  const Pen dithered_pen(Layout::ScalePenWidth(2), COLOR_BLACK);
  if (IsDithered())
    canvas.Select(dithered_pen);
  else
    canvas.SelectBlackPen();

  PolygonRotateShift(arrow, pt, angle, scale.arrow_scale);
  canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));

  canvas.SelectHollowBrush();
  canvas.DrawCircle(pt, scale.circle_radius);

  canvas.SelectBlackPen();
}
