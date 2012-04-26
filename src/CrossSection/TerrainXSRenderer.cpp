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
#include "Terrain/RasterBuffer.hpp"

void
TerrainXSRenderer::Draw(Canvas &canvas, const ChartRenderer &chart, const short *elevations) const
{
  const fixed max_distance = chart.GetXMax();

  RasterPoint points[2 + CrossSectionRenderer::NUM_SLICES];

  points[0] = chart.ToScreen(max_distance, fixed(-500));
  points[1] = chart.ToScreen(fixed_zero, fixed(-500));

  unsigned num_points = 2;
  for (unsigned j = 0; j < CrossSectionRenderer::NUM_SLICES; ++j) {
    const fixed distance_factor =
        fixed(j) / (CrossSectionRenderer::NUM_SLICES - 1);
    const fixed distance = distance_factor * max_distance;

    short h = elevations[j];
    if (RasterBuffer::IsSpecial(h)) {
      if (RasterBuffer::IsWater(h))
        /* water is at 0m MSL */
        /* XXX paint in blue? */
        h = 0;
      else
        /* skip "unknown" values */
        continue;
    }

    points[num_points++] = chart.ToScreen(distance, fixed(h));
  }

  if (num_points >= 4) {
    canvas.SelectNullPen();
    canvas.Select(look.terrain_brush);
    canvas.polygon(points, num_points);
  }
}
