/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "jasper/jas_seq.h"
#include "io/BufferedOutputStream.hxx"
#include "io/BufferedReader.hxx"

#include <algorithm>

#include <stdlib.h>

void
RasterTile::SaveCache(BufferedOutputStream &os) const
{
  MetaData data;
  data.start = start;
  data.end = end;

  os.Write(&data, sizeof(data));
}

void
RasterTile::LoadCache(BufferedReader &r)
{
  MetaData data;
  r.ReadFull({&data, sizeof(data)});
  Set(data.start, data.end);
}

void
RasterTile::CopyFrom(const struct jas_matrix &m) noexcept
{
  if (!IsDefined())
    return;

  buffer.Resize(size);

  auto *gcc_restrict dest = buffer.GetData();
  assert(dest != nullptr);

  const unsigned width = m.numcols_, height = m.numrows_;

  for (unsigned y = 0; y != height; ++y) {
    const jas_seqent_t *gcc_restrict src = m.rows_[y];

    for (unsigned i = 0; i < width; ++i)
      *dest++ = TerrainHeight(src[i]);
  }
}

TerrainHeight
RasterTile::GetHeight(RasterLocation p) const noexcept
{
  assert(IsLoaded());

  p -= start;

  assert(p.x < size.x);
  assert(p.y < size.y);

  return buffer.Get(p);
}

TerrainHeight
RasterTile::GetInterpolatedHeight(unsigned lx, unsigned ly,
                                  unsigned ix, unsigned iy) const noexcept
{
  assert(IsLoaded());

  // we want to exit out of this function as soon as possible
  // if we have the wrong tile

  // check x in range
  if ((lx -= start.x) >= size.x)
    return TerrainHeight::Invalid();

  // check y in range
  if ((ly -= start.y) >= size.y)
    return TerrainHeight::Invalid();

  return buffer.GetInterpolated(lx, ly, ix, iy);
}

inline unsigned
RasterTile::CalcDistanceTo(IntPoint2D p) const noexcept
{
  const unsigned int dx1 = abs(p.x - (int)start.x);
  const unsigned int dx2 = abs((int)end.x - p.x);
  const unsigned int dy1 = abs(p.y - (int)start.y);
  const unsigned int dy2 = abs((int)end.y - p.y);

  return std::max(std::min(dx1, dx2), std::min(dy1, dy2));
}

inline bool
RasterTile::CheckTileVisibility(IntPoint2D view,
                                unsigned view_radius) noexcept
{
  if (!IsDefined()) {
    assert(!IsLoaded());
    return false;
  }

  distance = CalcDistanceTo(view);
  return distance <= view_radius || IsLoaded();
}

bool
RasterTile::VisibilityChanged(IntPoint2D view,
                              unsigned view_radius) noexcept
{
  request = false;
  return CheckTileVisibility(view, view_radius);
}
