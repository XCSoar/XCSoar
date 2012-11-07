/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "AircraftRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Look/AircraftLook.hpp"
#include "Math/Screen.hpp"
#include "MapSettings.hpp"
#include "Util/Macros.hpp"
#include "Math/Angle.hpp"

static void
DrawMirroredPolygon(const RasterPoint *src, RasterPoint *dst, unsigned points,
                    Canvas &canvas, const Angle angle,
                    const RasterPoint pos)
{
  std::copy(src, src + points, dst);
  for (unsigned i = 0; i < points; ++i) {
    dst[2 * points - i - 1].x = -dst[i].x;
    dst[2 * points - i - 1].y = dst[i].y;
  }
  PolygonRotateShift(dst, 2 * points, pos.x, pos.y, angle, 50);
  canvas.DrawPolygon(dst, 2 * points);
}


static void
DrawDetailedAircraft(Canvas &canvas, bool inverse,
                     const AircraftLook &look,
                     const Angle angle,
                     const RasterPoint aircraft_pos)
{
  {
    static constexpr RasterPoint Aircraft[] = {
      {0, -10},
      {-2, -7},
      {-2, -2},
      {-16, -2},
      {-32, -1},
      {-32, 2},
      {-1, 3},
      {-1, 15},
      {-3, 15},
      {-5, 17},
      {-5, 18},
      {0, 18},
    };
    static constexpr unsigned AIRCRAFT_POINTS = ARRAY_SIZE(Aircraft);
    RasterPoint buffer[2 * AIRCRAFT_POINTS];

    if (!inverse) {
      canvas.SelectWhiteBrush();
      canvas.Select(look.aircraft_pen);
    } else {
      canvas.SelectBlackBrush();
      canvas.SelectWhitePen();
    }

    DrawMirroredPolygon(Aircraft, buffer, AIRCRAFT_POINTS,
                        canvas, angle, aircraft_pos);
  }

  {
    static constexpr RasterPoint Canopy[] = {
      {0, -7},
      {-1, -7},
      {-1, -2},
      {0, -1},
    };
    const unsigned CANOPY_POINTS = ARRAY_SIZE(Canopy);
    RasterPoint buffer[2 * CANOPY_POINTS];

    canvas.Select(look.canopy_pen);
    canvas.Select(look.canopy_brush);
    DrawMirroredPolygon(Canopy, buffer, CANOPY_POINTS,
                        canvas, angle, aircraft_pos);
  }
}


static void
DrawSimpleAircraft(Canvas &canvas, const AircraftLook &look,
                   const Angle angle,
                   const RasterPoint aircraft_pos, bool large)
{
  static constexpr RasterPoint AircraftLarge[] = {
    {1, -7},
    {1, -1},
    {17, -1},
    {17, 1},
    {1, 1},
    {1, 10},
    {5, 10},
    {5, 12},
    {-5, 12},
    {-5, 10},
    {-1, 10},
    {-1, 1},
    {-17, 1},
    {-17, -1},
    {-1, -1},
    {-1, -7},
  };

  static constexpr RasterPoint AircraftSmall[] = {
    {1, -5},
    {1, 0},
    {14, 0},
    {14, 1},
    {1, 1},
    {1, 8},
    {4, 8},
    {4, 9},
    {-3, 9},
    {-3, 8},
    {0, 8},
    {0, 1},
    {-13, 1},
    {-13, 0},
    {0, 0},
    {0, -5},
   };

  static constexpr unsigned AIRCRAFT_POINTS_LARGE = ARRAY_SIZE(AircraftLarge);
  static constexpr unsigned AIRCRAFT_POINTS_SMALL = ARRAY_SIZE(AircraftSmall);
  static const unsigned AIRCRAFT_POINTS_MAX =
    std::max(AIRCRAFT_POINTS_LARGE, AIRCRAFT_POINTS_SMALL);

  const RasterPoint *Aircraft = large ? AircraftLarge : AircraftSmall;
  const unsigned AircraftPoints = large ?
                                  AIRCRAFT_POINTS_LARGE : AIRCRAFT_POINTS_SMALL;

  RasterPoint aircraft[AIRCRAFT_POINTS_MAX];
  std::copy(Aircraft, Aircraft + AircraftPoints, aircraft);
  PolygonRotateShift(aircraft, AircraftPoints,
                     aircraft_pos.x, aircraft_pos.y, angle);
  canvas.SelectHollowBrush();
  canvas.Select(look.aircraft_simple2_pen);
  canvas.DrawPolygon(aircraft, AircraftPoints);
  canvas.SelectBlackBrush();
  canvas.Select(look.aircraft_simple1_pen);
  canvas.DrawPolygon(aircraft, AircraftPoints);
}

static void
DrawHangGlider(Canvas &canvas, const AircraftLook &look,
               const Angle angle, const RasterPoint aircraft_pos, bool inverse)
{
  RasterPoint aircraft[] = {
    {1, -3},
    {7, 0},
    {13, 4},
    {13, 6},
    {6, 3},
    {1, 2},
    {-1, 2},
    {-6, 3},
    {-13, 6},
    {-13, 4},
    {-7, 0},
    {-1, -3},
   };

  PolygonRotateShift(aircraft, ARRAY_SIZE(aircraft),
                     aircraft_pos.x, aircraft_pos.y, angle);

  if (inverse) {
    canvas.SelectBlackBrush();
    canvas.SelectWhitePen();
  } else {
    canvas.SelectWhiteBrush();
    canvas.SelectBlackPen();
  }

  canvas.DrawPolygon(aircraft, ARRAY_SIZE(aircraft));
}

static void
DrawParaGlider(Canvas &canvas, const AircraftLook &look,
               const Angle angle, const RasterPoint aircraft_pos, bool inverse)
{
  RasterPoint aircraft[] = {
    // Wing
    {-16, 5},
    {-22, 4},
    {-28, 2},
    {-27, -1},
    {-26, -2},
    {-24, -3},
    {-21, -4},
    {-16, -5},
    {0, -6},
    {16, -5},
    {21, -4},
    {24, -3},
    {26, -2},
    {27, -1},
    {28, 2},
    {22, 4},
    {16, 5},

    // Arrow on wing
    {4, 6},
    {-4, 6},
    {0, -4},
   };

  PolygonRotateShift(aircraft, ARRAY_SIZE(aircraft),
                     aircraft_pos.x, aircraft_pos.y, angle, 50);

  if (inverse) {
    canvas.SelectBlackBrush();
    canvas.SelectWhitePen();
  } else {
    canvas.SelectWhiteBrush();
    canvas.SelectBlackPen();
  }

  canvas.DrawPolygon(aircraft, ARRAY_SIZE(aircraft) - 1);

  canvas.SelectNullPen();
  if (inverse)
    canvas.SelectWhiteBrush();
  else
    canvas.SelectBlackBrush();

  canvas.DrawPolygon(aircraft + ARRAY_SIZE(aircraft) - 3, 3);
}

void
AircraftRenderer::Draw(Canvas &canvas, const MapSettings &settings_map,
                       const AircraftLook &look,
                       const Angle angle, const RasterPoint aircraft_pos)
{
  switch (settings_map.aircraft_symbol) {
  case AircraftSymbol::DETAILED:
    DrawDetailedAircraft(canvas, !settings_map.terrain.enable,
                         look, angle, aircraft_pos);
    break;

  case AircraftSymbol::SIMPLE_LARGE:
    DrawSimpleAircraft(canvas, look, angle, aircraft_pos, true);
    break;

  case AircraftSymbol::SIMPLE:
    DrawSimpleAircraft(canvas, look, angle, aircraft_pos, false);
    break;

  case AircraftSymbol::HANGGLIDER:
    DrawHangGlider(canvas, look, angle, aircraft_pos,
                   !settings_map.terrain.enable);
    break;

  case AircraftSymbol::PARAGLIDER:
    DrawParaGlider(canvas, look, angle, aircraft_pos,
                   !settings_map.terrain.enable);
    break;
  }
}
