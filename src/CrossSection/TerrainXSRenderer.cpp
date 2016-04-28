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

#include "TerrainXSRenderer.hpp"
#include "CrossSectionRenderer.hpp"
#include "Renderer/ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Look/CrossSectionLook.hpp"
#include "Util/StaticArray.hxx"

void
TerrainXSRenderer::Draw(Canvas &canvas, const ChartRenderer &chart,
                        const TerrainHeight *elevations) const
{
  const auto max_distance = chart.GetXMax();

  StaticArray<BulkPixelPoint, CrossSectionRenderer::NUM_SLICES + 2> points;

  canvas.SelectNullPen();

  TerrainType last_type = TerrainType::UNKNOWN;
  double last_distance = 0;
  const double hmin = chart.GetYMin();

  for (unsigned j = 0; j < CrossSectionRenderer::NUM_SLICES; ++j) {
    const auto distance_factor =
        double(j) / (CrossSectionRenderer::NUM_SLICES - 1);
    const auto distance = distance_factor * max_distance;

    const TerrainHeight e = elevations[j];
    const TerrainType type = e.GetType();

    // Close and paint polygon
    if (j != 0 &&
        type != last_type &&
        last_type != TerrainType::UNKNOWN) {
      const auto center_distance = (distance + last_distance) / 2;
      points.append() = chart.ToScreen(center_distance, hmin);
      points.append() = chart.ToScreen(center_distance, hmin);

      DrawPolygon(canvas, last_type, points.begin(), points.size());
    }

    if (type != TerrainType::UNKNOWN) {
      const double h = std::max((double)e.GetValueOr0(), hmin);

      if (j == 0) {
        // Start first polygon
        points.append() = chart.ToScreen(distance, hmin);
        points.append() = chart.ToScreen(distance, h);
      } else if (type != last_type) {
        // Start new polygon
        points.clear();

        const auto center_distance = (distance + last_distance) / 2;
        points.append() = chart.ToScreen(center_distance, hmin);
        points.append() = chart.ToScreen(center_distance, hmin);
      }

      if (j + 1 == CrossSectionRenderer::NUM_SLICES) {
        // Close and paint last polygon
        points.append() = chart.ToScreen(distance, h);
        points.append() = chart.ToScreen(distance, hmin);

        DrawPolygon(canvas, type, points.begin(), points.size());
      } else if (type == last_type && j != 0) {
        // Add single point to polygon
        points.append() = chart.ToScreen(distance, h);
      }
    }

    last_type = type;
    last_distance = distance;
  }
}

void
TerrainXSRenderer::DrawPolygon(Canvas &canvas, TerrainType type,
                               const BulkPixelPoint *points,
                               unsigned num_points) const
{
  assert(type != TerrainType::UNKNOWN);

  canvas.Select(type == TerrainType::WATER ?
                look.sea_brush : look.terrain_brush);
  canvas.DrawPolygon(points, num_points);
}
