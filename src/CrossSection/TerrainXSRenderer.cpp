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

#include "TerrainXSRenderer.hpp"
#include "CrossSectionRenderer.hpp"
#include "Renderer/ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Look/CrossSectionLook.hpp"
#include "Util/StaticArray.hpp"

void
TerrainXSRenderer::Draw(Canvas &canvas, const ChartRenderer &chart, const short *elevations) const
{
  const fixed max_distance = chart.GetXMax();

  StaticArray<RasterPoint, CrossSectionRenderer::NUM_SLICES + 2> points;

  canvas.SelectNullPen();

  RasterBuffer::TerrainType last_type = RasterBuffer::TerrainType::UNKNOWN;
  fixed last_distance = fixed(0);

  for (unsigned j = 0; j < CrossSectionRenderer::NUM_SLICES; ++j) {
    const fixed distance_factor =
        fixed(j) / (CrossSectionRenderer::NUM_SLICES - 1);
    const fixed distance = distance_factor * max_distance;

    short h = elevations[j];
    RasterBuffer::TerrainType type = RasterBuffer::GetTerrainType(h);

    if (type == RasterBuffer::TerrainType::WATER)
      h = 0;

    // Close and paint polygon
    if (j != 0 &&
        type != last_type &&
        last_type != RasterBuffer::TerrainType::UNKNOWN) {
      const fixed center_distance = (distance + last_distance) / 2;
      points.append() = chart.ToScreen(center_distance, fixed(0));
      points.append() = chart.ToScreen(center_distance, fixed(-500));

      DrawPolygon(canvas, last_type, points.begin(), points.size());
    }

    if (type != RasterBuffer::TerrainType::UNKNOWN) {
      if (j == 0) {
        // Start first polygon
        points.append() = chart.ToScreen(distance, fixed(-500));
        points.append() = chart.ToScreen(distance, fixed(h));
      } else if (type != last_type) {
        // Start new polygon
        points.clear();

        const fixed center_distance = (distance + last_distance) / 2;
        points.append() = chart.ToScreen(center_distance, fixed(-500));
        points.append() = chart.ToScreen(center_distance, fixed(0));
      }

      if (j + 1 == CrossSectionRenderer::NUM_SLICES) {
        // Close and paint last polygon
        points.append() = chart.ToScreen(distance, fixed(h));
        points.append() = chart.ToScreen(distance, fixed(-500));

        DrawPolygon(canvas, type, points.begin(), points.size());
      } else if (type == last_type && j != 0) {
        // Add single point to polygon
        points.append() = chart.ToScreen(distance, fixed(h));
      }
    }

    last_type = type;
    last_distance = distance;
  }
}

void
TerrainXSRenderer::DrawPolygon(Canvas &canvas, RasterBuffer::TerrainType type,
                               const RasterPoint *points,
                               unsigned num_points) const
{
  assert(type != RasterBuffer::TerrainType::UNKNOWN);

  canvas.Select(type == RasterBuffer::TerrainType::WATER ?
                look.sea_brush : look.terrain_brush);
  canvas.DrawPolygon(points, num_points);
}
