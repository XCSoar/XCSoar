// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RasterTileCache.hpp"
#include "Math/Angle.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/BufferedReader.hxx"
#include "util/SpanCast.hxx"

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

/**
 * How many samples fit in @p n source pixels, starting at @p phase
 * and stepping by @p skip.
 */
static constexpr unsigned
SubsampleCount(unsigned n, unsigned phase, unsigned skip) noexcept
{
  if (n <= phase || skip == 0)
    return 0;

  return 1u + (n - 1u - phase) / skip;
}

/**
 * Copy one sample from the middle of each coarse block.  The sample
 * stays in the same block a fine-tile lookup uses, so loading the
 * tile does not slide the terrain east or south.
 */
static void
PutSubsampledTile(RasterBuffer &dest, unsigned shift,
                  RasterLocation start, const struct jas_matrix &m) noexcept
{
  const unsigned dest_pitch = dest.GetSize().x;
  const unsigned skip = 1u << shift;
  const unsigned prefer = RasterTraits::SubsamplePhase(shift);
  const unsigned numcols = unsigned(m.numcols_);
  const unsigned numrows = unsigned(m.numrows_);
  const unsigned phase_x = prefer < numcols ? prefer : 0u;
  const unsigned phase_y = prefer < numrows ? prefer : 0u;

  start.x >>= shift;
  start.y >>= shift;

  if (start.x >= dest.GetSize().x || start.y >= dest.GetSize().y)
    return;

  unsigned width = SubsampleCount(numcols, phase_x, skip);
  if (start.x + width > dest.GetSize().x)
    width = dest.GetSize().x - start.x;
  unsigned height = SubsampleCount(numrows, phase_y, skip);
  if (start.y + height > dest.GetSize().y)
    height = dest.GetSize().y - start.y;

  auto *gcc_restrict out = dest.GetData()
    + start.y * dest_pitch + start.x;

  for (unsigned i = 0, y = phase_y; i < height;
       ++i, y += skip, out += dest_pitch)
    CopyOverviewRow(out, m.rows_[y] + phase_x, width, skip);
}

void
RasterTileCache::PutOverviewTile(unsigned index,
                                 RasterLocation start, RasterLocation end,
                                 const struct jas_matrix &m) noexcept
{
  tiles.GetLinear(index).Set(start, end);

  PutSubsampledTile(step, RasterTraits::STEP_BITS, start, m);
  PutSubsampledTile(overview, RasterTraits::OVERVIEW_BITS, start, m);
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

  /* One JPEG2000 pass walks the file from the start.  Request every
     visible tile in that pass.  Repeating the walk for a handful of
     tiles costs more than decoding them together.  MAX_ACTIVE_TILES
     still bounds how many are held in memory. */
  constexpr unsigned MAX_ACTIVATE = MAX_ACTIVE_TILES;

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

  // still not found, so go to the step grid
  constexpr unsigned shift =
    RasterTraits::SUBPIXEL_BITS - RasterTraits::STEP_BITS;
  return step.GetInterpolated(p << shift);
}

TerrainHeight
RasterTileCache::GetInterpolatedHeight(RasterLocation l) const noexcept
{
  if (l.x >= overview_size_fine.x || l.y >= overview_size_fine.y)
    // outside overall bounds
    return TerrainHeight::Invalid();

  const auto [px, ix] = RasterTraits::CalcSubpixel(l.x);
  const auto [py, iy] = RasterTraits::CalcSubpixel(l.y);

  const RasterTile &tile = tiles.Get(px / tile_size.x, py / tile_size.y);
  if (tile.IsLoaded())
    return tile.GetInterpolatedHeight(px, py, ix, iy);

  // still not found, so go to the step grid
  return step.GetInterpolated({RasterTraits::ToStep(l.x),
                               RasterTraits::ToStep(l.y)});
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
  overview.Resize({RasterTraits::ToOverviewCeil(size.x),
                   RasterTraits::ToOverviewCeil(size.y)});
  step.Resize({RasterTraits::ToStepCeil(size.x),
               RasterTraits::ToStepCeil(size.y)});
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
  step.Reset();

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

  os.Write(ReferenceAsBytes(header));
  os.Write(std::as_bytes(std::span{segments}));

  /* save tiles */
  unsigned i;
  for (i = 0; i < tiles.GetSize(); ++i) {
    const auto &tile = tiles.GetLinear(i);
    if (tile.IsDefined()) {
      os.Write(ReferenceAsBytes(i));
      tile.SaveCache(os);
    }
  }

  i = -1;
  os.Write(ReferenceAsBytes(i));

  /* save overview, then the finer step grid */
  size_t overview_size = overview.GetSize().Area();
  os.Write(std::as_bytes(std::span{overview.GetData(), overview_size}));

  size_t step_size = step.GetSize().Area();
  os.Write(std::as_bytes(std::span{step.GetData(), step_size}));
}

void
RasterTileCache::LoadCache(BufferedReader &r)
{
  Reset();

  /* load metadata */
  const auto header = r.ReadFullT<CacheHeader>();

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
    segments.append() = r.ReadFullT<MarkerSegmentInfo>();
  }

  /* load tiles */
  while (true) {
    const auto i = r.ReadFullT<unsigned>();

    if (i == (unsigned)-1)
      break;

    if (i >= tiles.GetSize())
      throw std::runtime_error("Bad tile index");

    tiles.GetLinear(i).LoadCache(r);
  }

  /* load overview, then the finer step grid */
  size_t overview_size = overview.GetSize().Area();
  r.ReadFull(std::as_writable_bytes(std::span{
        overview.GetData(),
        overview_size,
      }));

  size_t step_size = step.GetSize().Area();
  r.ReadFull(std::as_writable_bytes(std::span{
        step.GetData(),
        step_size,
      }));
}
