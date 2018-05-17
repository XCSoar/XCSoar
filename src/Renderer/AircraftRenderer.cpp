/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "RotatedPolygonRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Look/AircraftLook.hpp"
#include "MapSettings.hpp"
#include "Asset.hpp"
#include "Math/Angle.hpp"

#include <algorithm>

static void
DrawMirroredPolygon(const BulkPixelPoint *src, unsigned points,
                    Canvas &canvas, const Angle angle,
                    const PixelPoint pos)
{
  BulkPixelPoint dst[64];
  assert(2 * points <= ARRAY_SIZE(dst));

  std::copy_n(src, points, dst);
  for (unsigned i = 0; i < points; ++i) {
    dst[2 * points - i - 1].x = -dst[i].x;
    dst[2 * points - i - 1].y = dst[i].y;
  }
#ifdef ENABLE_OPENGL
  CanvasRotateShift rotate_shift(pos, angle, 50);
#else
  PolygonRotateShift(dst, 2 * points, pos, angle, 50);
#endif
  canvas.DrawPolygon(dst, 2 * points);
}

static void
DrawDetailedAircraft(Canvas &canvas, bool inverse,
                     const AircraftLook &look,
                     const Angle angle,
                     const PixelPoint aircraft_pos)
{
  {
    static constexpr BulkPixelPoint Aircraft[] = {
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

    if (!inverse) {
      canvas.SelectWhiteBrush();
      canvas.Select(look.aircraft_pen);
    } else {
      canvas.SelectBlackBrush();
      canvas.SelectWhitePen();
    }

    DrawMirroredPolygon(Aircraft, AIRCRAFT_POINTS,
                        canvas, angle, aircraft_pos);
  }

  {
    static constexpr BulkPixelPoint Canopy[] = {
      {0, -7},
      {-1, -7},
      {-1, -2},
      {0, -1},
    };
    const unsigned CANOPY_POINTS = ARRAY_SIZE(Canopy);

    canvas.Select(look.canopy_pen);
    canvas.Select(look.canopy_brush);
    DrawMirroredPolygon(Canopy, CANOPY_POINTS,
                        canvas, angle, aircraft_pos);
  }
}


static void
DrawSimpleAircraft(Canvas &canvas, const AircraftLook &look,
                   const Angle angle,
                   const PixelPoint aircraft_pos, bool large)
{
  static constexpr BulkPixelPoint AircraftLarge[] = {
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

  static constexpr BulkPixelPoint AircraftSmall[] = {
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

  const auto *Aircraft = large ? AircraftLarge : AircraftSmall;
  const unsigned AircraftPoints = large ?
                                  AIRCRAFT_POINTS_LARGE : AIRCRAFT_POINTS_SMALL;

  const RotatedPolygonRenderer renderer(Aircraft, AircraftPoints,
                                        aircraft_pos, angle);

  canvas.SelectHollowBrush();
  canvas.Select(look.aircraft_simple2_pen);
  renderer.Draw(canvas, 0, AircraftPoints);
  canvas.SelectBlackBrush();
  canvas.Select(look.aircraft_simple1_pen);
  renderer.Draw(canvas, 0, AircraftPoints);
}

static void
DrawHangGlider(Canvas &canvas, const AircraftLook &look,
               const Angle angle, const PixelPoint aircraft_pos, bool inverse)
{
  static constexpr BulkPixelPoint aircraft[] = {
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

  if (inverse) {
    canvas.SelectBlackBrush();
    canvas.SelectWhitePen();
  } else {
    canvas.SelectWhiteBrush();
    canvas.SelectBlackPen();
  }

  const RotatedPolygonRenderer renderer(aircraft, ARRAY_SIZE(aircraft),
                                        aircraft_pos, angle);
  renderer.Draw(canvas, 0, ARRAY_SIZE(aircraft));
}

static void
DrawParaGlider(Canvas &canvas, const AircraftLook &look,
               const Angle angle, const PixelPoint aircraft_pos, bool inverse)
{
  static constexpr BulkPixelPoint aircraft[] = {
    // Wing
    {-5, 3},
    {-11, 2},
    {-14, 1},
    {-14, -1},
    {-11, -2},
    {-5, -3},
    {5, -3},
    {11, -2},
    {14, -1},
    {14, 1},
    {11, 2},
    {5, 3},

    // Arrow on wing
    {2, 3},
    {-2, 3},
    {0, -3},
   };

  const RotatedPolygonRenderer renderer(aircraft, ARRAY_SIZE(aircraft),
                                        aircraft_pos, angle);

  if (inverse) {
    canvas.SelectBlackBrush();
    canvas.SelectWhitePen();
  } else {
    canvas.SelectWhiteBrush();
    canvas.SelectBlackPen();
  }

  renderer.Draw(canvas, 0, ARRAY_SIZE(aircraft) - 3);

  canvas.SelectNullPen();
  if (inverse)
    canvas.SelectWhiteBrush();
  else
    canvas.SelectBlackBrush();

  renderer.Draw(canvas, ARRAY_SIZE(aircraft) - 3, 3);
}

void
AircraftRenderer::Draw(Canvas &canvas, const MapSettings &settings_map,
                       const AircraftLook &look,
                       const Angle angle, const PixelPoint aircraft_pos)
{
  const bool inverse = IsDithered() || !settings_map.terrain.enable;

  switch (settings_map.aircraft_symbol) {
  case AircraftSymbol::DETAILED:
    DrawDetailedAircraft(canvas, inverse,
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
                   inverse);
    break;

  case AircraftSymbol::PARAGLIDER:
    DrawParaGlider(canvas, look, angle, aircraft_pos,
                   inverse);
    break;
  }
}
