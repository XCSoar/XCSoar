// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

  os.Write(std::as_bytes(std::span{&data, 1}));
}

void
RasterTile::LoadCache(BufferedReader &r)
{
  const auto data = r.ReadFullT<MetaData>();
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
