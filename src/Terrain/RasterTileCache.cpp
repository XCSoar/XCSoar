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

#include "RasterTileCache.hpp"
#include "Math/Angle.hpp"
#include "Math/FastMath.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/BufferedReader.hxx"

extern "C" {
#include "jasper/jas_seq.h"
}

#include <stdexcept>

#include <string.h>
#include <algorithm>

static void
CopyOverviewRow(TerrainHeight *gcc_restrict dest, const jas_seqent_t *gcc_restrict src,
                unsigned width, unsigned skip) noexcept
{
  /* note: this loop rounds up */
  for (unsigned x = 0; x < width; ++x, src += skip)
    *dest++ = TerrainHeight(*src);
}

void
RasterTileCache::PutOverviewTile(unsigned index,
                                 RasterLocation start, RasterLocation end,
                                 const struct jas_matrix &m) noexcept
{
  tiles.GetLinear(index).Set(start, end);

  const unsigned dest_pitch = overview.GetSize().x;

  start.x = RasterTraits::ToOverview(start.x);
  start.y = RasterTraits::ToOverview(start.y);

  if (start.x >= overview.GetSize().x || start.y >= overview.GetSize().y)
    return;

  unsigned width = RasterTraits::ToOverviewCeil(m.numcols_);
  if (start.x + width > overview.GetSize().x)
    width = overview.GetSize().x - start.x;
  unsigned height = RasterTraits::ToOverviewCeil(m.numrows_);
  if (start.y + height > overview.GetSize().y)
    height = overview.GetSize().y - start.y;

  const unsigned skip = 1 << OVERVIEW_BITS;

  auto *gcc_restrict dest = overview.GetData()
    + start.y * dest_pitch + start.x;

  /* note: this loop rounds up */
  for (unsigned i = 0, y = 0; i < height; ++i, y += skip, dest += dest_pitch)
    CopyOverviewRow(dest, m.rows_[y], width, skip);
}

void
RasterTileCache::PutTileData(unsigned index,
                             const struct jas_matrix &m) noexcept
{
  auto &tile = tiles.GetLinear(index);
  if (!tile.IsRequested())
    return;

  tile.CopyFrom(m);
}

struct RTDistanceSort {
  const RasterTileCache &rtc;

  constexpr RTDistanceSort(RasterTileCache &_rtc) noexcept:rtc(_rtc) {}

  [[gnu::pure]]
  bool operator()(unsigned short ai, unsigned short bi) const noexcept {
    const RasterTile &a = rtc.tiles.GetLinear(ai);
    const RasterTile &b = rtc.tiles.GetLinear(bi);

    return a.GetDistance() < b.GetDistance();
  }
};

bool
RasterTileCache::PollTiles(SignedRasterLocation p, unsigned radius) noexcept
{
  /* tiles are usually 256 pixels wide; with a radius smaller than
     that, the (optimized) tile distance calculations may fail;
     additionally, this ensures that tiles which are slightly out of
     the screen will be loaded in advance */
  radius += 256;

  /**
   * Maximum number of tiles loaded at a time, to reduce system load
   * peaks.
   */
  constexpr unsigned MAX_ACTIVATE = MAX_ACTIVE_TILES > 32
    ? 16
    : MAX_ACTIVE_TILES / 2;

  /* query all tiles; all tiles which are either in range or already
     loaded are added to RequestTiles */

  request_tiles.clear();
  for (int i = tiles.GetSize() - 1; i >= 0 && !request_tiles.full(); --i)
    if (tiles.GetLinear(i).VisibilityChanged(p, radius))
      request_tiles.append(i);

  /* reduce if there are too many */

  if (request_tiles.size() > MAX_ACTIVE_TILES) {
    /* sort by distance */
    const RTDistanceSort sort(*this);
    std::sort(request_tiles.begin(), request_tiles.end(), sort);

    /* dispose all tiles which are out of range */
    for (unsigned i = MAX_ACTIVE_TILES; i < request_tiles.size(); ++i) {
      RasterTile &tile = tiles.GetLinear(request_tiles[i]);
      tile.Unload();
    }

    request_tiles.shrink(MAX_ACTIVE_TILES);
  }

  /* fill ActiveTiles and request new tiles */

  dirty = false;

  unsigned num_activate = 0;
  for (unsigned i = 0; i < request_tiles.size(); ++i) {
    RasterTile &tile = tiles.GetLinear(request_tiles[i]);
    if (tile.IsLoaded())
      continue;

    if (++num_activate <= MAX_ACTIVATE)
      /* request the tile in the current iteration */
      tile.SetRequest();
    else
      /* this tile will be loaded in the next iteration */
      dirty = true;
  }

  return num_activate > 0;
}

TerrainHeight
RasterTileCache::GetHeight(RasterLocation p) const noexcept
{
  if (p.x >= size.x || p.y >= size.y)
    // outside overall bounds
    return TerrainHeight::Invalid();

  const RasterTile &tile = tiles.Get(p.x / tile_size.x, p.y / tile_size.y);
  if (tile.IsLoaded())
    return tile.GetHeight(p);

  // still not found, so go to overview
  return overview.GetInterpolated(p << (RasterTraits::SUBPIXEL_BITS - RasterTraits::OVERVIEW_BITS));
}

TerrainHeight
RasterTileCache::GetInterpolatedHeight(RasterLocation l) const noexcept
{
  if (l.x >= overview_size_fine.x || l.y >= overview_size_fine.y)
    // outside overall bounds
    return TerrainHeight::Invalid();

  unsigned px = l.x, py = l.y;
  const unsigned int ix = CombinedDivAndMod(px);
  const unsigned int iy = CombinedDivAndMod(py);

  const RasterTile &tile = tiles.Get(px / tile_size.x, py / tile_size.y);
  if (tile.IsLoaded())
    return tile.GetInterpolatedHeight(px, py, ix, iy);

  // still not found, so go to overview
  return overview.GetInterpolated({RasterTraits::ToOverview(l.x), RasterTraits::ToOverview(l.y)});
}

void
RasterTileCache::SetSize(UnsignedPoint2D _size,
                         Point2D<uint_least16_t> _tile_size,
                         UnsignedPoint2D _n_tiles) noexcept
{
  size = _size;
  tile_size = _tile_size;

  /* round the overview size up, because PutOverviewTile() does the
     same */
  overview.Resize({RasterTraits::ToOverviewCeil(size.x), RasterTraits::ToOverviewCeil(size.y)});
  overview_size_fine = size << RasterTraits::SUBPIXEL_BITS;

  tiles.GrowDiscard(_n_tiles.x, _n_tiles.y);
}

void
RasterTileCache::SetLatLonBounds(double _lon_min, double _lon_max,
                                 double _lat_min, double _lat_max) noexcept
{
  const Angle lon_min(Angle::Degrees(_lon_min));
  const Angle lon_max(Angle::Degrees(_lon_max));
  const Angle lat_min(Angle::Degrees(_lat_min));
  const Angle lat_max(Angle::Degrees(_lat_max));

  bounds = GeoBounds(GeoPoint(std::min(lon_min, lon_max),
                              std::max(lat_min, lat_max)),
                     GeoPoint(std::max(lon_min, lon_max),
                              std::min(lat_min, lat_max)));
}

void
RasterTileCache::Reset() noexcept
{
  size = {0, 0};
  bounds.SetInvalid();
  segments.clear();

  overview.Reset();

  for (auto &i : tiles)
    i.Unload();
}

const RasterTileCache::MarkerSegmentInfo *
RasterTileCache::FindMarkerSegment(uint32_t file_offset) const noexcept
{
  for (const auto &s : segments)
    if (s.file_offset >= file_offset)
      return &s;

  return nullptr;
}

void
RasterTileCache::FinishTileUpdate() noexcept
{
  /* permanently disable the requested tiles which are still not
     loaded, to prevent trying to reload them over and over in a busy
     loop */
  for (std::size_t i : request_tiles) {
    RasterTile &tile = tiles.GetLinear(i);
    if (tile.IsRequested() && !tile.IsLoaded())
      tile.Clear();
  }

  ++serial;
}

void
RasterTileCache::SaveCache(BufferedOutputStream &os) const
{
  if (!IsValid())
    throw std::runtime_error("Terrain invalid");

  assert(bounds.IsValid());

  /* save metadata */
  CacheHeader header;

  /* zero-fill all implicit padding bytes (to make valgrind happy) */
  memset(&header, 0, sizeof(header));

  header.version = CacheHeader::VERSION;
  header.size = size;
  header.tile_size = tile_size;
  header.n_tiles = {tiles.GetWidth(), tiles.GetHeight()};
  header.num_marker_segments = segments.size();
  header.bounds = bounds;

  os.Write(&header, sizeof(header));
  os.Write(segments.begin(), sizeof(*segments.begin()) * segments.size());

  /* save tiles */
  unsigned i;
  for (i = 0; i < tiles.GetSize(); ++i) {
    const auto &tile = tiles.GetLinear(i);
    if (tile.IsDefined()) {
      os.Write(&i, sizeof(i));
      tile.SaveCache(os);
    }
  }

  i = -1;
  os.Write(&i, sizeof(i));

  /* save overview */
  size_t overview_size = overview.GetSize().Area();
  os.Write(overview.GetData(), sizeof(*overview.GetData()) * overview_size);
}

void
RasterTileCache::LoadCache(BufferedReader &r)
{
  Reset();

  /* load metadata */
  CacheHeader header;
  r.ReadFull({&header, sizeof(header)});

  if (header.version != CacheHeader::VERSION ||
      header.size.x < 1024 || header.size.x > 1024 * 1024 ||
      header.size.y < 1024 || header.size.y > 1024 * 1024 ||
      header.tile_size.x < 16 || header.tile_size.x > 16 * 1024 ||
      header.tile_size.y < 16 || header.tile_size.y > 16 * 1024 ||
      header.n_tiles.x < 1 || header.n_tiles.x > 1024 ||
      header.n_tiles.y < 1 || header.n_tiles.y > 1024 ||
      header.num_marker_segments < 4 ||
      header.num_marker_segments > segments.capacity() ||
      header.bounds.IsEmpty())
    throw std::runtime_error("Malformed terrain cache header");

  SetSize(header.size, header.tile_size, header.n_tiles);
  bounds = header.bounds;
  if (!bounds.IsValid())
    throw std::runtime_error("Malformed terrain cache bounds");

  /* load segments */
  for (unsigned i = 0; i < header.num_marker_segments; ++i) {
    MarkerSegmentInfo &segment = segments.append();
    r.ReadFull({&segment, sizeof(segment)});
  }

  /* load tiles */
  while (true) {
    unsigned i;
    r.ReadFull({&i, sizeof(i)});

    if (i == (unsigned)-1)
      break;

    if (i >= tiles.GetSize())
      throw std::runtime_error("Bad tile index");

    tiles.GetLinear(i).LoadCache(r);
  }

  /* load overview */
  size_t overview_size = overview.GetSize().Area();
  r.ReadFull({
      overview.GetData(),
      sizeof(*overview.GetData()) * overview_size,
    });
}
