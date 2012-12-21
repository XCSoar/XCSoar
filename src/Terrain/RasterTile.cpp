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

#include "Terrain/RasterTile.hpp"

#include <algorithm>

bool
RasterTile::SaveCache(FILE *file) const
{
  MetaData data;
  data.xstart = xstart;
  data.ystart = ystart;
  data.xend = xend;
  data.yend = yend;

  return fwrite(&data, sizeof(data), 1, file) == 1;
}

bool
RasterTile::LoadCache(FILE *file)
{
  MetaData data;
  if (fread(&data, sizeof(data), 1, file) != 1)
    return false;

  Set(data.xstart, data.ystart, data.xend, data.yend);
  return true;
}

void
RasterTile::Enable()
{
  if (!width || !height) {
    Disable();
  } else {
    buffer.Resize(width, height);
  }
}

short
RasterTile::GetHeight(unsigned x, unsigned y) const
{
  assert(IsEnabled());

  x -= xstart;
  y -= ystart;

  assert(x < width);
  assert(y < height);

  return buffer.Get(x, y);
}

short
RasterTile::GetInterpolatedHeight(unsigned lx, unsigned ly,
                                  unsigned ix, unsigned iy) const
{
  // we want to exit out of this function as soon as possible
  // if we have the wrong tile

  if (IsDisabled())
    return RasterBuffer::TERRAIN_INVALID;

  // check x in range
  if ((lx -= xstart) >= width)
    return RasterBuffer::TERRAIN_INVALID;

  // check y in range
  if ((ly -= ystart) >= height)
    return RasterBuffer::TERRAIN_INVALID;

  return buffer.GetInterpolated(lx, ly, ix, iy);
}

bool
RasterTile::CheckTileVisibility(int view_x, int view_y, unsigned view_radius)
{
  if (!width || !height) {
    Disable();
    return false;
  }

  const unsigned int dx1 = abs(view_x - (int)xstart);
  const unsigned int dx2 = abs((int)xend - view_x);
  const unsigned int dy1 = abs(view_y - (int)ystart);
  const unsigned int dy2 = abs((int)yend - view_y);

  distance = std::max(std::min(dx1, dx2), std::min(dy1, dy2));
  return distance <= view_radius || IsEnabled();
}

bool
RasterTile::VisibilityChanged(int view_x, int view_y, unsigned view_radius)
{
  request = false;
  return CheckTileVisibility(view_x, view_y, view_radius);
}
