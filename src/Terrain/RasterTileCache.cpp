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

#include "RasterTileCache.hpp"
#include "Math/Angle.hpp"
#include "Math/FastMath.hpp"

extern "C" {
#include "jasper/jas_seq.h"
}

#include <string.h>
#include <algorithm>

static void
CopyOverviewRow(TerrainHeight *gcc_restrict dest, const jas_seqent_t *gcc_restrict src,
                unsigned width, unsigned skip)
{
  for (unsigned x = 0; x < width; ++x, src += skip)
    *dest++ = TerrainHeight(*src);
}

void
RasterTileCache::PutOverviewTile(unsigned index,
                                 unsigned start_x, unsigned start_y,
                                 unsigned end_x, unsigned end_y,
                                 const struct jas_matrix &m)
{
  tiles.GetLinear(index).Set(start_x, start_y, end_x, end_y);

  const unsigned dest_pitch = overview.GetWidth();

  start_x = RasterTraits::ToOverview(start_x);
  start_y = RasterTraits::ToOverview(start_y);

  if (start_x >= overview.GetWidth() || start_y >= overview.GetHeight())
    return;

  unsigned width = RasterTraits::ToOverviewCeil(m.numcols_);
  if (start_x + width > overview.GetWidth())
    width = overview.GetWidth() - start_x;
  unsigned height = RasterTraits::ToOverviewCeil(m.numrows_);
  if (start_y + height > overview.GetHeight())
    height = overview.GetHeight() - start_y;

  const unsigned skip = 1 << OVERVIEW_BITS;

  auto *gcc_restrict dest = overview.GetData()
    + start_y * dest_pitch + start_x;

  for (unsigned i = 0, y = 0; i < height; ++i, y += skip, dest += dest_pitch)
    CopyOverviewRow(dest, m.rows_[y], width, skip);
}

void
RasterTileCache::PutTileData(unsigned index,
                             const struct jas_matrix &m)
{
  auto &tile = tiles.GetLinear(index);
  if (!tile.IsRequested())
    return;

  tile.CopyFrom(m);
}

struct RTDistanceSort {
  const RasterTileCache &rtc;

  RTDistanceSort(RasterTileCache &_rtc):rtc(_rtc) {}

  bool operator()(unsigned short ai, unsigned short bi) const {
    const RasterTile &a = rtc.tiles.GetLinear(ai);
    const RasterTile &b = rtc.tiles.GetLinear(bi);

    return a.GetDistance() < b.GetDistance();
  }
};

bool
RasterTileCache::PollTiles(int x, int y, unsigned radius)
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
    if (tiles.GetLinear(i).VisibilityChanged(x, y, radius))
      request_tiles.append(i);

  /* reduce if there are too many */

  if (request_tiles.size() > MAX_ACTIVE_TILES) {
    /* sort by distance */
    const RTDistanceSort sort(*this);
    std::sort(request_tiles.begin(), request_tiles.end(), sort);

    /* dispose all tiles which are out of range */
    for (unsigned i = MAX_ACTIVE_TILES; i < request_tiles.size(); ++i) {
      RasterTile &tile = tiles.GetLinear(request_tiles[i]);
      tile.Disable();
    }

    request_tiles.shrink(MAX_ACTIVE_TILES);
  }

  /* fill ActiveTiles and request new tiles */

  dirty = false;

  unsigned num_activate = 0;
  for (unsigned i = 0; i < request_tiles.size(); ++i) {
    RasterTile &tile = tiles.GetLinear(request_tiles[i]);
    if (tile.IsEnabled())
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
RasterTileCache::GetHeight(unsigned px, unsigned py) const
{
  if (px >= width || py >= height)
    // outside overall bounds
    return TerrainHeight::Invalid();

  const RasterTile &tile = tiles.Get(px / tile_width, py / tile_height);
  if (tile.IsEnabled())
    return tile.GetHeight(px, py);

  // still not found, so go to overview
  return overview.GetInterpolated(px << (RasterTraits::SUBPIXEL_BITS - RasterTraits::OVERVIEW_BITS),
                                  py << (RasterTraits::SUBPIXEL_BITS - RasterTraits::OVERVIEW_BITS));
}

TerrainHeight
RasterTileCache::GetInterpolatedHeight(unsigned int lx, unsigned int ly) const
{
  if ((lx >= overview_width_fine) || (ly >= overview_height_fine))
    // outside overall bounds
    return TerrainHeight::Invalid();

  unsigned px = lx, py = ly;
  const unsigned int ix = CombinedDivAndMod(px);
  const unsigned int iy = CombinedDivAndMod(py);

  const RasterTile &tile = tiles.Get(px / tile_width, py / tile_height);
  if (tile.IsEnabled())
    return tile.GetInterpolatedHeight(px, py, ix, iy);

  // still not found, so go to overview
  return overview.GetInterpolated(RasterTraits::ToOverview(lx),
                                  RasterTraits::ToOverview(ly));
}

void
RasterTileCache::SetSize(unsigned _width, unsigned _height,
                         unsigned _tile_width, unsigned _tile_height,
                         unsigned tile_columns, unsigned tile_rows)
{
  width = _width;
  height = _height;
  tile_width = _tile_width;
  tile_height = _tile_height;

  overview.Resize(RasterTraits::ToOverview(width),
                  RasterTraits::ToOverview(height));
  overview_width_fine = width << RasterTraits::SUBPIXEL_BITS;
  overview_height_fine = height << RasterTraits::SUBPIXEL_BITS;

  tiles.GrowDiscard(tile_columns, tile_rows);
}

void
RasterTileCache::SetLatLonBounds(double _lon_min, double _lon_max,
                                 double _lat_min, double _lat_max)
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
RasterTileCache::Reset()
{
  width = 0;
  height = 0;
  bounds.SetInvalid();
  segments.clear();

  overview.Reset();

  for (auto it = tiles.begin(), end = tiles.end(); it != end; ++it)
    it->Disable();
}

const RasterTileCache::MarkerSegmentInfo *
RasterTileCache::FindMarkerSegment(uint32_t file_offset) const
{
  for (const auto &s : segments)
    if (s.file_offset >= file_offset)
      return &s;

  return nullptr;
}

void
RasterTileCache::FinishTileUpdate()
{
  /* permanently disable the requested tiles which are still not
     loaded, to prevent trying to reload them over and over in a busy
     loop */
  for (auto it = request_tiles.begin(), end = request_tiles.end();
      it != end; ++it) {
    RasterTile &tile = tiles.GetLinear(*it);
    if (tile.IsRequested() && !tile.IsEnabled())
      tile.Clear();
  }

  ++serial;
}

bool
RasterTileCache::SaveCache(FILE *file) const
{
  if (!IsValid())
    return false;

  assert(bounds.IsValid());

  /* save metadata */
  CacheHeader header;

  /* zero-fill all implicit padding bytes (to make valgrind happy) */
  memset(&header, 0, sizeof(header));

  header.version = CacheHeader::VERSION;
  header.width = width;
  header.height = height;
  header.tile_width = tile_width;
  header.tile_height = tile_height;
  header.tile_columns = tiles.GetWidth();
  header.tile_rows = tiles.GetHeight();
  header.num_marker_segments = segments.size();
  header.bounds = bounds;

  if (fwrite(&header, sizeof(header), 1, file) != 1 ||
      /* .. and segments */
      fwrite(segments.begin(), sizeof(*segments.begin()), segments.size(), file) != segments.size())
    return false;

  /* save tiles */
  unsigned i;
  for (i = 0; i < tiles.GetSize(); ++i)
    if (tiles.GetLinear(i).IsDefined() &&
        (fwrite(&i, sizeof(i), 1, file) != 1 ||
         !tiles.GetLinear(i).SaveCache(file)))
      return false;

  i = -1;
  if (fwrite(&i, sizeof(i), 1, file) != 1)
    return false;

  /* save overview */
  size_t overview_size = overview.GetWidth() * overview.GetHeight();
  if (fwrite(overview.GetData(), sizeof(*overview.GetData()),
             overview_size, file) != overview_size)
    return false;

  /* done */
  return true;
}

bool
RasterTileCache::LoadCache(FILE *file)
{
  Reset();

  /* load metadata */
  CacheHeader header;
  if (fread(&header, sizeof(header), 1, file) != 1 ||
      header.version != CacheHeader::VERSION ||
      header.width < 1024 || header.width > 1024 * 1024 ||
      header.height < 1024 || header.height > 1024 * 1024 ||
      header.num_marker_segments < 4 ||
      header.num_marker_segments > segments.capacity() ||
      header.bounds.IsEmpty())
    return false;

  SetSize(header.width, header.height,
          header.tile_width, header.tile_height,
          header.tile_columns, header.tile_rows);
  bounds = header.bounds;
  if (!bounds.IsValid())
    return false;

  /* load segments */
  for (unsigned i = 0; i < header.num_marker_segments; ++i) {
    MarkerSegmentInfo &segment = segments.append();
    if (fread(&segment, sizeof(segment), 1, file) != 1)
      return false;
  }

  /* load tiles */
  unsigned i;
  while (true) {
    if (fread(&i, sizeof(i), 1, file) != 1)
      return false;

    if (i == (unsigned)-1)
      break;

    if (i >= tiles.GetSize())
      return false;

    if (!tiles.GetLinear(i).LoadCache(file))
      return false;
  }

  /* load overview */
  size_t overview_size = overview.GetWidth() * overview.GetHeight();
  if (fread(overview.GetData(), sizeof(*overview.GetData()),
            overview_size, file) != overview_size)
    return false;

  return true;
}
