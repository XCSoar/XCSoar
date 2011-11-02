/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Terrain/RasterLocation.hpp"
#include "jasper/jas_image.h"
#include "Math/Angle.hpp"
#include "IO/ZipLineReader.hpp"
#include "Operation.hpp"
#include "Math/FastMath.h"

#include <stdlib.h>
#include <algorithm>
#include <limits.h>

using std::min;
using std::max;


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

  set(data.xstart, data.ystart, data.xend, data.yend);
  return true;
}

void
RasterTile::Enable()
{
  if (!width || !height) {
    Disable();
  } else {
    buffer.resize(width, height);
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

  return buffer.get(x, y);
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

  return buffer.get_interpolated(lx, ly, ix, iy);
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

short*
RasterTileCache::GetImageBuffer(unsigned index)
{
  if (TileRequest(index))
    return tiles.GetLinear(index).GetImageBuffer();

  return NULL;
}

void
RasterTileCache::SetTile(unsigned index,
                         int xstart, int ystart, int xend, int yend)
{
  if (!segments.empty() && segments.last().tile < 0)
    /* link current marker segment with this tile */
    segments.last().tile = index;

  tiles.GetLinear(index).set(xstart, ystart, xend, yend);
}

struct RTDistanceSort {
  const RasterTileCache &rtc;

  RTDistanceSort(RasterTileCache &_rtc):rtc(_rtc) {}

  bool operator()(unsigned short ai, unsigned short bi) const {
    const RasterTile &a = rtc.tiles.GetLinear(ai);
    const RasterTile &b = rtc.tiles.GetLinear(bi);

    return a.get_distance() < b.get_distance();
  }
};

bool
RasterTileCache::PollTiles(int x, int y, unsigned radius)
{
  if (scan_overview)
    return false;

  /* tiles are usually 256 pixels wide; with a radius smaller than
     that, the (optimized) tile distance calculations may fail;
     additionally, this ensures that tiles which are slightly out of
     the screen will be loaded in advance */
  radius += 256;

  enum {
    /**
     * Maximum number of tiles loaded at a time, to reduce system load
     * peaks.
    */
    MAX_ACTIVATE = MAX_ACTIVE_TILES > 32 ? 16 : MAX_ACTIVE_TILES / 2,
  };

  /* query all tiles; all tiles which are either in range or already
     loaded are added to RequestTiles */

  RequestTiles.clear();
  for (int i = tiles.GetSize() - 1; i >= 0; --i)
    if (tiles.GetLinear(i).VisibilityChanged(x, y, radius))
      RequestTiles.append(i);

  /* reduce if there are too many */

  if (RequestTiles.size() > MAX_ACTIVE_TILES) {
    /* sort by distance */
    const RTDistanceSort sort(*this);
    std::sort(RequestTiles.begin(), RequestTiles.end(), sort);

    /* dispose all tiles which are out of range */
    for (unsigned i = MAX_ACTIVE_TILES; i < RequestTiles.size(); ++i) {
      RasterTile &tile = tiles.GetLinear(RequestTiles[i]);
      tile.Disable();
    }

    RequestTiles.shrink(MAX_ACTIVE_TILES);
  }

  /* fill ActiveTiles and request new tiles */

  dirty = false;

  unsigned num_activate = 0;
  for (unsigned i = 0; i < RequestTiles.size(); ++i) {
    RasterTile &tile = tiles.GetLinear(RequestTiles[i]);
    if (tile.IsEnabled())
      continue;

    if (++num_activate <= MAX_ACTIVATE)
      /* request the tile in the current iteration */
      tile.set_request();
    else
      /* this tile will be loaded in the next iteration */
      dirty = true;
  }

  return num_activate > 0;
}

bool
RasterTileCache::TileRequest(unsigned index)
{
  RasterTile &tile = tiles.GetLinear(index);

  if (!tile.is_requested())
    return false;

  tile.Enable();
  return true; // want to load this one!
}

short
RasterTileCache::GetHeight(unsigned px, unsigned py) const
{
  if (px >= width || py >= height)
    // outside overall bounds
    return RasterBuffer::TERRAIN_INVALID;

  const RasterTile &tile = tiles.Get(px / tile_width, py / tile_height);
  if (tile.IsEnabled())
    return tile.GetHeight(px, py);

  // still not found, so go to overview
  return Overview.get_interpolated(px << (SUBPIXEL_BITS - OVERVIEW_BITS),
                                   py << (SUBPIXEL_BITS - OVERVIEW_BITS));
}

short
RasterTileCache::GetInterpolatedHeight(unsigned int lx, unsigned int ly) const
{
  if ((lx >= overview_width_fine) || (ly >= overview_height_fine))
    // outside overall bounds
    return RasterBuffer::TERRAIN_INVALID;

  unsigned px = lx, py = ly;
  const unsigned int ix = CombinedDivAndMod(px);
  const unsigned int iy = CombinedDivAndMod(py);

  const RasterTile &tile = tiles.Get(px / tile_width, py / tile_height);
  if (tile.IsEnabled())
    return tile.GetInterpolatedHeight(px, py, ix, iy);

  // still not found, so go to overview
  return Overview.get_interpolated(lx >> OVERVIEW_BITS,
                                   ly >> OVERVIEW_BITS);
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

  Overview.resize(width >> OVERVIEW_BITS, height >> OVERVIEW_BITS);
  overview_width_fine = width << SUBPIXEL_BITS;
  overview_height_fine = height << SUBPIXEL_BITS;

  tiles.GrowDiscard(tile_columns, tile_rows);
}

void
RasterTileCache::SetLatLonBounds(double _lon_min, double _lon_max,
                                 double _lat_min, double _lat_max)
{
  bounds.west = Angle::Degrees(fixed(min(_lon_min, _lon_max)));
  bounds.east = Angle::Degrees(fixed(max(_lon_min, _lon_max)));
  bounds.north = Angle::Degrees(fixed(max(_lat_min, _lat_max)));
  bounds.south = Angle::Degrees(fixed(min(_lat_min, _lat_max)));
  bounds_initialised = true;
}

void
RasterTileCache::Reset()
{
  width = 0;
  height = 0;
  initialised = false;
  bounds_initialised = false;
  segments.clear();
  scan_overview = true;

  Overview.reset();

  for (auto it = tiles.begin(), end = tiles.end(); it != end; ++it)
    it->Disable();
}

gcc_pure
const RasterTileCache::MarkerSegmentInfo *
RasterTileCache::FindMarkerSegment(long file_offset) const
{
  for (const MarkerSegmentInfo *p = segments.begin(); p < segments.end(); ++p)
    if (p->file_offset >= file_offset)
      return p;

  return NULL;
}

long
RasterTileCache::SkipMarkerSegment(long file_offset) const
{
  if (scan_overview)
    /* use all segments when loading the overview */
    return 0;

  const MarkerSegmentInfo *segment = FindMarkerSegment(file_offset);
  if (segment == NULL)
    /* past the end of the recorded segment list; shouldn't happen */
    return 0;

  long skip_to = segment->file_offset;
  while (segment->tile >= 0 &&
         !tiles.GetLinear(segment->tile).is_requested()) {
    ++segment;
    if (segment >= segments.end())
      /* last segment is hidden; shouldn't happen either, because we
         expect EOC there */
      break;

    skip_to = segment->file_offset;
  }

  return skip_to - file_offset;
}

/**
 * Does this segment belong to the preceding tile?  If yes, then it
 * inherits the tile number.
 */
static bool
is_tile_segment(unsigned id)
{
  return id == 0xff93 /* SOD */ ||
    id == 0xff52 /* COD */ ||
    id == 0xff53 /* COC */ ||
    id == 0xff5c /* QCD */ ||
    id == 0xff5d /* QCC */ ||
    id == 0xff5e /* RGN */ ||
    id == 0xff5f /* POC */ ||
    id == 0xff61 /* PPT */ ||
    id == 0xff58 /* PLT */ ||
    id == 0xff64 /* COM */;
}

void
RasterTileCache::MarkerSegment(long file_offset, unsigned id)
{
  if (!scan_overview || segments.full())
    return;

  if (operation != NULL)
    operation->SetProgressPosition(file_offset / 65536);

  int tile = -1;
  if (is_tile_segment(id) && !segments.empty())
    /* this segment belongs to the same tile as the preceding SOT
       segment */
    tile = segments.last().tile;

  segments.append(MarkerSegmentInfo(file_offset, tile));
}

extern RasterTileCache *raster_tile_current;

void
RasterTileCache::LoadJPG2000(const char *jp2_filename)
{
  jas_stream_t *in;

  raster_tile_current = this;

  in = jas_stream_fopen(jp2_filename, "rb");
  if (!in) {
    Reset();
    return;
  }

  if (operation != NULL)
    operation->SetProgressRange(jas_stream_length(in) / 65536);

  jp2_decode(in, scan_overview ? "xcsoar=2" : "xcsoar=1");
  jas_stream_close(in);
}

bool
RasterTileCache::LoadWorldFile(const TCHAR *path)
{
  ZipLineReaderA reader(path);
  if (reader.error())
    return false;

  char *endptr;
  const char *line = reader.read(); // x scale
  double x_scale = strtod(line, &endptr);
  if (endptr == line)
    return false;

  line = reader.read(); // y rotation
  if (line == NULL)
    return false;

  double y_rotation = strtod(line, &endptr);
  if (endptr == line || y_rotation < -0.01 || y_rotation > 0.01)
    /* we don't support rotation */
    return false;

  line = reader.read(); // x rotation
  if (line == NULL)
    return false;

  double x_rotation = strtod(line, &endptr);
  if (endptr == line || x_rotation < -0.01 || x_rotation > 0.01)
    /* we don't support rotation */
    return false;

  line = reader.read(); // y scale
  if (line == NULL)
    return false;

  double y_scale = strtod(line, &endptr);
  if (endptr == line)
    return false;

  line = reader.read(); // x origin
  if (line == NULL)
    return false;

  double x_origin = strtod(line, &endptr);
  if (endptr == line)
    return false;

  line = reader.read(); // y origin
  if (line == NULL)
    return false;

  double y_origin = strtod(line, &endptr);
  if (endptr == line)
    return false;

  SetLatLonBounds(x_origin, x_origin + GetWidth() * x_scale,
                  y_origin, y_origin + GetHeight() * y_scale);
  return true;
}

bool
RasterTileCache::LoadOverview(const char *path, const TCHAR *world_file,
                              OperationEnvironment &_operation)
{
  assert(operation == NULL);
  operation = &_operation;

  Reset();

  LoadJPG2000(path);
  scan_overview = false;

  if (initialised && world_file != NULL)
    LoadWorldFile(world_file);

  if (initialised && !bounds_initialised)
    initialised = false;

  if (!initialised)
    Reset();

  operation = NULL;
  return initialised;
}

void
RasterTileCache::UpdateTiles(const char *path, int x, int y, unsigned radius)
{
  if (!PollTiles(x, y, radius))
    return;

  LoadJPG2000(path);

  /* permanently disable the requested tiles which are still not
     loaded, to prevent trying to reload them over and over in a busy
     loop */
  for (auto it = RequestTiles.begin(), end = RequestTiles.end();
      it != end; ++it) {
    RasterTile &tile = tiles.GetLinear(*it);
    if (tile.is_requested() && !tile.IsEnabled())
      tile.Clear();
  }
}

bool
RasterTileCache::SaveCache(FILE *file) const
{
  if (!initialised)
    return false;

  assert(bounds_initialised);

  /* save metadata */
  CacheHeader header;
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
    if (tiles.GetLinear(i).defined() &&
        (fwrite(&i, sizeof(i), 1, file) != 1 ||
         !tiles.GetLinear(i).SaveCache(file)))
      return false;

  i = -1;
  if (fwrite(&i, sizeof(i), 1, file) != 1)
    return false;

  /* save overview */
  size_t overview_size = Overview.get_width() * Overview.get_height();
  if (fwrite(Overview.get_data(), sizeof(*Overview.get_data()),
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
      header.bounds.empty())
    return false;

  SetSize(header.width, header.height,
          header.tile_width, header.tile_height,
          header.tile_columns, header.tile_rows);
  bounds = header.bounds;
  bounds_initialised = true;

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
  size_t overview_size = Overview.get_width() * Overview.get_height();
  if (fread(Overview.get_data(), sizeof(*Overview.get_data()),
            overview_size, file) != overview_size)
    return false;

  initialised = true;
  scan_overview = false;
  return true;
}

struct GridLocation : public RasterLocation {
  unsigned short tile_x, tile_y;
  unsigned remainder_x, remainder_y;
  unsigned index;

  GridLocation(const RasterLocation &other,
               unsigned tile_width, unsigned tile_height,
               unsigned _index)
    :RasterLocation(other),
     tile_x(other.x / tile_width), tile_y(other.y / tile_height),
     remainder_x(other.x % tile_width),
     remainder_y(other.y % tile_height),
     index(_index) {}
};

struct GridRay {
  unsigned tile_width, tile_height;

  GridLocation start, end;

  int delta_x, delta_y;

  unsigned size;

  GridRay(unsigned _tile_width, unsigned _tile_height,
          const RasterLocation &_start, const RasterLocation &_end,
          unsigned _size)
    :tile_width(_tile_width), tile_height(_tile_height),
     start(_start, tile_width, tile_height, 0),
     end(_end, tile_width, tile_height, _size),
     delta_x(end.x - start.x), delta_y(end.y - start.y),
     size(_size) {}
};

gcc_pure
static int
ScaleX(const GridRay &ray, int x, int range)
{
  assert(ray.delta_x != 0);

  return (int64_t)(x - (int)ray.start.x) * (int64_t)range / ray.delta_x;
}

gcc_pure
static int
YAtX(const GridRay &ray, int x)
{
  assert(ray.delta_x != 0);

  return (int)ray.start.y + ScaleX(ray, x, ray.delta_y);
}

gcc_pure
static unsigned
IndexAtX(const GridRay &ray, int x)
{
  assert(ray.delta_x != 0);

  return ScaleX(ray, x, ray.size);
}

gcc_pure
static GridLocation
NextRightGridIntersection(const GridRay &ray,
                          const GridLocation &current)
{
  assert(ray.delta_x > 0);

  if (current.tile_x == ray.end.tile_x)
    return ray.end;

  assert(current.tile_x < ray.end.tile_x);
  assert(current.x < ray.end.x);

  int x = (current.tile_x + 1) * ray.tile_width;
  int y = YAtX(ray, x);
  assert(y >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_width, ray.tile_height,
                      IndexAtX(ray, x));
}

gcc_pure
static GridLocation
NextLeftGridIntersection(const GridRay &ray, GridLocation current)
{
  assert(ray.delta_x < 0);

  if (current.tile_x == ray.end.tile_x)
    return ray.end;

  assert(current.tile_x > ray.end.tile_x);
  assert(current.x > ray.end.x);

  unsigned tile_x = current.tile_x;
  if (current.remainder_x == 0) {
    --tile_x;
    if (tile_x == ray.end.tile_x)
      return ray.end;
  }

  int x = tile_x * ray.tile_width;
  int y = YAtX(ray, x);
  assert(y >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_width, ray.tile_height,
                      IndexAtX(ray, x));
}

gcc_pure
static GridLocation
NextHorizontalGridIntersection(const GridRay &ray,
                               const GridLocation &current)
{
  if (ray.delta_x > 0)
    return NextRightGridIntersection(ray, current);
  else if (ray.delta_x < 0)
    return NextLeftGridIntersection(ray, current);
  else
    return ray.end;
}

gcc_pure
static int
ScaleY(const GridRay &ray, int y, int range)
{
  assert(ray.delta_y != 0);

  return (int64_t)(y - (int)ray.start.y) * (int64_t)range / ray.delta_y;
}

gcc_pure
static int
XAtY(const GridRay &ray, int y)
{
  assert(ray.delta_y != 0);

  return (int)ray.start.x + ScaleY(ray, y, ray.delta_x);
}

gcc_pure
static unsigned
IndexAtY(const GridRay &ray, int y)
{
  assert(ray.delta_y != 0);

  return ScaleY(ray, y, ray.size);
}

gcc_pure
static GridLocation
NextBottomGridIntersection(const GridRay &ray,
                           const GridLocation &current)
{
  assert(ray.delta_y > 0);

  if (current.tile_y == ray.end.tile_y)
    return ray.end;

  assert(current.tile_y < ray.end.tile_y);
  assert(current.y < ray.end.y);

  int y = (current.tile_y + 1) * ray.tile_height;
  int x = XAtY(ray, y);
  assert(x >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_width, ray.tile_height,
                      IndexAtY(ray, y));
}

gcc_pure
static GridLocation
NextTopGridIntersection(const GridRay &ray, GridLocation current)
{
  assert(ray.delta_y < 0);

  if (current.tile_y == ray.end.tile_y)
    return ray.end;

  assert(current.tile_y > ray.end.tile_y);
  assert(current.y > ray.end.y);

  unsigned tile_y = current.tile_y;
  if (current.remainder_y == 0) {
    --tile_y;
    if (tile_y == ray.end.tile_y)
      return ray.end;
  }

  int y = tile_y * ray.tile_height;
  int x = XAtY(ray, y);
  assert(x >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_width, ray.tile_height,
                      IndexAtY(ray, y));
}

gcc_pure
static GridLocation
NextVerticalGridIntersection(const GridRay &ray,
                             const GridLocation &current)
{
  assert(ray.start.index == 0);
  assert(ray.end.index == ray.size);

  if (ray.delta_y > 0)
    return NextBottomGridIntersection(ray, current);
  else if (ray.delta_y < 0)
    return NextTopGridIntersection(ray, current);
  else
    return ray.end;
}

gcc_pure
static GridLocation
NextGridIntersection(const GridRay &ray,
                     const GridLocation &current)
{
  assert(ray.start.index == 0);
  assert(ray.end.index > 0);

  GridLocation h = NextHorizontalGridIntersection(ray, current);
  GridLocation v = NextVerticalGridIntersection(ray, current);

  return abs((int)(h.x - ray.start.x)) + abs((int)(h.y - ray.start.y)) <
    abs((int)(v.x - ray.start.x)) + abs((int)(v.y - ray.start.y))
    ? h : v;
}

void
RasterTileCache::ScanTileLine(GridLocation start, GridLocation end,
                              short *buffer, unsigned size,
                              bool interpolate) const
{
  assert(end.index >= start.index);
  assert(end.index <= size);

  if (start.index == end.index)
    return;

  if (start.tile_x < end.tile_x) {
    assert(end.tile_x == start.tile_x + 1);
    assert(end.remainder_x == 0);

    --end.x;
  } else if (start.tile_x > end.tile_x) {
    assert(start.tile_x == end.tile_x + 1);
    assert(start.remainder_x == 0);

    --start.x;
    --start.tile_x;
  }

  if (start.tile_y < end.tile_y) {
    assert(end.tile_y == start.tile_y + 1);
    assert(end.remainder_y == 0);

    --end.y;
  } else if (start.tile_y > end.tile_y) {
    assert(start.tile_y == end.tile_y + 1);
    assert(start.remainder_y == 0);

    --start.y;
    --start.tile_y;
  }

  const RasterTile &tile = tiles.Get(start.tile_x, start.tile_y);
  if (tile.IsEnabled())
    tile.ScanLine(start.x, start.y, end.x, end.y,
                  buffer + start.index, end.index - start.index,
                  interpolate);
  else
    /* need range checking in the overview buffer because its size may
       be rounded down, and then the "fine" location may exceed its
       bounds */
    Overview.ScanLineChecked(start.x >> OVERVIEW_BITS,
                             start.y >> OVERVIEW_BITS,
                             end.x >> OVERVIEW_BITS, end.y >> OVERVIEW_BITS,
                             buffer + start.index, end.index - start.index,
                             interpolate);
}

void
RasterTileCache::ScanLine(const RasterLocation _start,
                          const RasterLocation _end,
                          short *buffer, unsigned size, bool interpolate) const
{
  assert(_start.x < width << 8);
  assert(_start.y < height << 8);
  assert(_end.x < width << 8);
  assert(_end.y < height << 8);
  assert(size >= 2);

  const GridRay ray(tile_width << 8, tile_height << 8, _start, _end, size);
  assert(ray.size == size);
  assert(ray.start.index == 0);
  assert(ray.end.index == size);

  GridLocation current = ray.start;
  while (current.index < size) {
    GridLocation next = NextGridIntersection(ray, current);
    ScanTileLine(current, next, buffer, size, interpolate);
    current = next;
  }
}

//#define DEBUG_TILE
#ifdef DEBUG_TILE
#include <stdio.h>
#endif

bool
RasterTileCache::FirstIntersection(int x0, int y0,
                                   int x1, int y1,
                                   short h_origin,
                                   short h_dest,
                                   const int slope_fact, const short h_ceiling,
                                   const short h_safety,
                                   unsigned& _x, unsigned& _y, short &_h,
                                   const bool can_climb) const
{
  if (((unsigned)x0 >= width) || ((unsigned)y0 >= height))
    // origin is outside overall bounds
    return false;

  // remember index of active tile, so we dont need to scan each each time
  // (unless the guess fails)
  int tile_index = -1;

  h_origin = std::max(h_origin, GetFieldDirect(x0, y0, tile_index));
  h_dest = std::max(h_dest, h_origin);

  // line algorithm parameters
  const int dx = abs(x1-x0);
  const int dy = abs(y1-y0);
  int err = dx-dy;
  const int sx = (x0 < x1)? 1: -1;
  const int sy = (y0 < y1)? 1: -1;

  // max number of steps to walk
  const int max_steps = (dx+dy);
  // calculate number of fine steps to produce a step on the overview field
  const int step_fine = std::max(1, max_steps >> INTERSECT_BITS);
  // number of steps for update to the overview map
  const int step_coarse = std::max(1<< OVERVIEW_BITS, step_fine);

  // number of steps to be cleared after climbing over obstruction
  const int intersect_steps = 32;

  // counter for steps to reach next position to be checked on the field.
  unsigned step_counter = 0;
  // total counter of fine steps
  int total_steps = 0;

  // number of steps since intersection
  int intersect_counter = 0;

#ifdef DEBUG_TILE
  printf("# max steps %d\n", max_steps);
  printf("# step coarse %d\n", step_coarse);
  printf("# step fine %d\n", step_fine);
#endif

  // early exit if origin is too high (should not occur)
  if (h_origin> h_ceiling) {
#ifdef DEBUG_TILE
    printf("# fint start above ceiling %d %d\n", h_origin, h_ceiling);
#endif
    _x = (unsigned)x0;
    _y = (unsigned)y0;
    _h = h_origin;
    return true;
  }

#ifdef DEBUG_TILE
  printf("# fint width %d height %d\n", width, height);
#endif
  short h_terrain = 1;

  unsigned x_int= x0, y_int= y0;
  short h_int= h_origin;
  // location of last point within ceiling limit that doesnt intersect
  unsigned last_clear_x = x0;
  unsigned last_clear_y = y0;
  short last_clear_h = h_origin;

  while (h_terrain>=0) {

    if (!step_counter) {

      if ((_x >= width) || (_y >= height))
        break; // outside bounds

      h_terrain = GetFieldDirect(x_int, y_int, tile_index)+h_safety;
      step_counter = tile_index<0? step_coarse: step_fine;

      // calculate height of glide so far
      const short dh = (short)((total_steps*slope_fact)>>RASTER_SLOPE_FACT);

      // current aircraft height
      h_int = dh + h_origin;
      if (can_climb) {
        h_int = std::min(h_int, h_dest);
      }

#ifdef DEBUG_TILE
      printf("%d %d %d %d %d # fint\n", x_int, y_int, h_int, h_terrain, h_ceiling);
#endif

      // this point has intersected if aircraft is below terrain height
      const bool this_intersecting = (h_int< h_terrain);

      if (this_intersecting) {
        intersect_counter = 1;

        // when intersecting, consider origin to have started higher
        const short h_jump = h_terrain - h_int;
        h_origin += h_jump;

        if (can_climb) {
          // if intersecting beyond desired destination height, allow dest height
          // to be increased
          if (h_terrain> h_dest)
            h_dest = h_terrain;
        } else {
          // if can't climb, must jump so path is pure glide
          h_dest += h_jump;
        }
        h_int = h_terrain;

      }

      if (h_int > h_ceiling) {
        _x = last_clear_x;
        _y = last_clear_y;
        _h = last_clear_h;
#ifdef DEBUG_TILE
        printf("# fint reach ceiling\n");
#endif
        return true; // reached ceiling
      }

      if (!this_intersecting) {
        if (intersect_counter) {
          intersect_counter+= step_counter;

          // was intersecting, now cleared.
          // exit with small height above terrain
#ifdef DEBUG_TILE
          printf("# fint int->clear\n");
#endif
          if (intersect_counter >= intersect_steps) {
            _x = x_int;
            _y = y_int;
            _h = h_int;
            return true;
          }
        } else {
          last_clear_x = x_int;
          last_clear_y = y_int;
          last_clear_h = h_int;
        }
      }
    }

    if (!intersect_counter && (total_steps == max_steps)) {
#ifdef DEBUG_TILE
      printf("# fint cleared\n");
#endif
      return false;
    }

    const int e2 = 2*err;
    if (e2 > -dy) {
      err -= dy;
      x_int += sx;
      if (step_counter)
        step_counter--;
      total_steps++;
    }
    if (e2 < dx) {
      err += dx;
      y_int += sy;
      if (step_counter)
        step_counter--;
      total_steps++;
    }
  }

  // early exit due to inability to find clearance after intersecting
  if (intersect_counter) {
    _x = last_clear_x;
    _y = last_clear_y;
    _h = last_clear_h;
#ifdef DEBUG_TILE
    printf("# fint early exit\n");
#endif
    return true;
  }
  return false;
}

#define ACCURATE_TERRAIN_INTERSECTION

short
RasterTileCache::GetFieldDirect(const unsigned px, const unsigned py, int& tile_index) const
{
#ifdef ACCURATE_TERRAIN_INTERSECTION

  const RasterTile &tile = tiles.Get(px / tile_width, py / tile_height);
  if (tile.IsEnabled())
    return tile.GetHeight(px, py);

#endif

  // still not found, so go to overview
  tile_index = -1;

  if ((px >= width) || (py >= height))
    // outside overall bounds
    return RasterBuffer::TERRAIN_INVALID;

  return Overview.get(px >> OVERVIEW_BITS, py >> OVERVIEW_BITS);
}

RasterLocation
RasterTileCache::Intersection(int x0, int y0,
                              int x1, int y1,
                              short h_origin,
                              const int slope_fact) const
{
  unsigned _x = (unsigned)x0;
  unsigned _y = (unsigned)y0;

  if ((_x >= width) || (_y >= height)) {
    // origin is outside overall bounds
    return RasterLocation(_x, _y);
  }

  // remember index of active tile, so we dont need to scan each each time
  // (unless the guess fails)
  int tile_index = -1;

  // line algorithm parameters
  const int dx = abs(x1-x0);
  const int dy = abs(y1-y0);
  int err = dx-dy;
  const int sx = (x0 < x1)? 1: -1;
  const int sy = (y0 < y1)? 1: -1;

  // max number of steps to walk
  const int max_steps = (dx+dy);
  // calculate number of fine steps to produce a step on the overview field

  // step size at selected refinement level
  const int refine_step = max_steps >> 5;

  // number of steps for update to the fine map
  const int step_fine = std::max(1, refine_step);
  // number of steps for update to the overview map
  const int step_coarse = std::max(1<< OVERVIEW_BITS, step_fine);

  // counter for steps to reach next position to be checked on the field.
  unsigned step_counter = 0;
  // total counter of fine steps
  int total_steps = 0;

#ifdef DEBUG_TILE
  printf("# max steps %d\n", max_steps);
  printf("# step coarse %d\n", step_coarse);
  printf("# step fine %d\n", step_fine);
#endif

  short h_int= h_origin;
  short h_terrain = 1;

  unsigned last_clear_x = _x;
  unsigned last_clear_y = _y;
  short last_clear_h = h_int;

  while (h_terrain>=0) {

    if (!step_counter) {

      if ((_x >= width) || (_y >= height))
        break; // outside bounds

      h_terrain = GetFieldDirect(_x, _y, tile_index);
      step_counter = tile_index<0? step_coarse: step_fine;

      // calculate height of glide so far
      const short dh = (short)((total_steps*slope_fact)>>RASTER_SLOPE_FACT);

      // current aircraft height
      h_int = h_origin-dh;

      if (h_int < h_terrain) {
        if (refine_step<3) // can't refine any further
          return RasterLocation(last_clear_x, last_clear_y);

        // refine solution
        return Intersection(last_clear_x, last_clear_y, _x, _y, last_clear_h, slope_fact);
      }

      if (h_int <= 0) 
        break; // reached max range

      last_clear_x = _x;
      last_clear_y = _y;
      last_clear_h = h_int;
    }

    if (total_steps > max_steps)
      break;

    const int e2 = 2*err;
    if (e2 > -dy) {
      err -= dy;
      _x += sx;
      if (step_counter>0)
        step_counter--;
      total_steps++;
    }
    if (e2 < dx) {
      err += dx;
      _y += sy;
      if (step_counter>0)
        step_counter--;
      total_steps++;
    }
  }

  // if we reached invalid terrain, assume we can hit MSL
  return RasterLocation(x1, y1);
}
