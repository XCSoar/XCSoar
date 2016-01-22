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

#include "Loader.hpp"
#include "RasterTileCache.hpp"
#include "RasterProjection.hpp"
#include "ZzipStream.hpp"
#include "WorldFile.hpp"
#include "Operation/Operation.hpp"
#include "OS/ConvertPathName.hpp"

extern "C" {
#include "jasper/jp2/jp2_cod.h"
#include "jasper/jpc/jpc_dec.h"
#include "jasper/jpc/jpc_t1cod.h"
}

long
TerrainLoader::SkipMarkerSegment(long file_offset) const
{
  if (scan_overview)
    /* use all segments when loading the overview */
    return 0;

  if (remaining_segments > 0) {
    /* enable the follow-up segment */
    --remaining_segments;
    return 0;
  }

  const auto *segment = raster_tile_cache.FindMarkerSegment(file_offset);
  if (segment == nullptr)
    /* past the end of the recorded segment list; shouldn't happen */
    return 0;

  long skip_to = segment->file_offset;
  while (segment->IsTileSegment() &&
         !raster_tile_cache.tiles.GetLinear(segment->tile).IsRequested()) {
    ++segment;
    if (segment >= raster_tile_cache.segments.end())
      /* last segment is hidden; shouldn't happen either, because we
         expect EOC there */
      break;

    skip_to = segment->file_offset;
  }

  remaining_segments = segment->count;
  return skip_to - file_offset;
}

/**
 * Does this segment belong to the preceding tile?  If yes, then it
 * inherits the tile number.
 */
static constexpr bool
IsTileSegment(unsigned id)
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
TerrainLoader::MarkerSegment(long file_offset, unsigned id)
{
  auto &segments = raster_tile_cache.segments;

  if (!scan_overview || segments.full())
    return;

  env.SetProgressPosition(file_offset / 65536);

  if (IsTileSegment(id) && !segments.empty() &&
      segments.back().IsTileSegment()) {
    /* this segment belongs to the same tile as the preceding SOT
       segment */
    ++segments.back().count;
    return;
  }

  if (segments.size() >= 2 && !segments.back().IsTileSegment() &&
      !segments[segments.size() - 2].IsTileSegment()) {
    /* the last two segments are both "generic" segments and can be merged*/
    assert(segments.back().count == 0);

    ++segments[segments.size() - 2].count;

    /* reuse the second segment */
    segments.back().file_offset = file_offset;
  } else
    segments.append(RasterTileCache::MarkerSegmentInfo(file_offset,
                                                       RasterTileCache::MarkerSegmentInfo::NO_TILE));
}

inline void
TerrainLoader::ParseBounds(const char *data)
{
  /* this code is obsolete, since new map files include a "world
     file", but we keep it for compatibility */

  data = strstr(data, "XCSoar");
  if (data == nullptr)
    return;

  float lon_min, lon_max, lat_min, lat_max;
  if (sscanf(data + 6, "%f %f %f %f",
             &lon_min, &lon_max, &lat_min, &lat_max) == 4)
    raster_tile_cache.SetLatLonBounds(lon_min, lon_max, lat_min, lat_max);
}

void
TerrainLoader::ProcessComment(const char *data, unsigned size)
{
  if (scan_overview) {
    char buffer[128];
    if (size < sizeof(buffer)) {
      memcpy(buffer, data, size);
      buffer[size] = 0;
      ParseBounds(buffer);
    }
  }
}

void
TerrainLoader::StartTile(unsigned index)
{
  if (scan_overview)
    raster_tile_cache.StartTile(index);
}

void
TerrainLoader::SetSize(unsigned _width, unsigned _height,
                       unsigned _tile_width, unsigned _tile_height,
                       unsigned tile_columns, unsigned tile_rows)
{
  if (scan_overview)
    raster_tile_cache.SetSize(_width, _height, _tile_width, _tile_height,
                              tile_columns, tile_rows);
}

void
TerrainLoader::PutTileData(unsigned index,
                           unsigned start_x, unsigned start_y,
                           unsigned end_x, unsigned end_y,
                           const struct jas_matrix &m)
{
  if (scan_overview)
    raster_tile_cache.PutOverviewTile(index, start_x, start_y,
                                      end_x, end_y, m);

  if (scan_tiles) {
    const ScopeExclusiveLock lock(mutex);
    raster_tile_cache.PutTileData(index, m);
  }
}

static bool
LoadJPG2000(jas_stream_t *in, void *loader)
{
  /* Get the first box.  This should be a JP box. */
  auto box = jp2_box_get(in);
  if (box == nullptr)
    return false;

  if (box->type != JP2_BOX_JP ||
      box->data.jp.magic != JP2_JP_MAGIC) {
    jp2_box_destroy(box);
    return false;
  }

  jp2_box_destroy(box);

  /* Get the second box.  This should be a FTYP box. */
  box = jp2_box_get(in);
  if (box == nullptr)
    return false;

  auto type = box->type;
  jp2_box_destroy(box);
  if (type != JP2_BOX_FTYP)
    return false;

  /* find the JP2C box */
  do {
    box = jp2_box_get(in);
    if (box == nullptr)
      /* not found */
      return false;

    type = box->type;
    jp2_box_destroy(box);
  } while (type != JP2_BOX_JP2C);

  jpc_dec_importopts_t opts;
  opts.debug = 0;
  opts.maxlyrs = JPC_MAXLYRS;
  opts.maxpkts = -1;

  jpc_initluts();

  const auto dec = jpc_dec_create(&opts, in);
  if (dec == nullptr)
    return false;

  dec->loader = loader;

  bool success = jpc_dec_decode(dec) == 0;
  jpc_dec_destroy(dec);
  return success;
}

inline bool
TerrainLoader::LoadJPG2000(struct zzip_dir *dir, const char *path)
{
  const auto in = OpenJasperZzipStream(dir, path);
  if (in == nullptr)
    return false;

  env.SetProgressRange(jas_stream_length(in) / 65536);

  bool success = ::LoadJPG2000(in, this);
  jas_stream_close(in);
  return success;
}

static bool
LoadWorldFile(RasterTileCache &tile_cache,
              struct zzip_dir *dir, const char *path)
{
  if (path == nullptr)
    return false;

  const auto new_bounds = LoadWorldFile(dir, path, tile_cache.GetWidth(),
                                        tile_cache.GetHeight());
  bool success = new_bounds.IsValid();
  if (success)
    tile_cache.SetBounds(new_bounds);
  return success;
}

inline bool
TerrainLoader::LoadOverview(struct zzip_dir *dir,
                            const char *path, const char *world_file)
{
  assert(scan_overview);

  raster_tile_cache.Reset();

  bool success = LoadJPG2000(dir, path);

  /* if we loaded the JPG2000 file successfully, but no bounds were
     obtained from there, try to load the world file "terrain.j2w" */
  if (success && !raster_tile_cache.bounds.IsValid() &&
      !LoadWorldFile(raster_tile_cache, dir, world_file))
    /* that failed: without bounds, we can't do anything; give up,
       discard the whole file */
    success = false;

  if (!success)
    raster_tile_cache.Reset();

  return success;
}

bool
LoadTerrainOverview(struct zzip_dir *dir,
                    const char *path, const char *world_file,
                    RasterTileCache &raster_tile_cache,
                    bool all,
                    OperationEnvironment &env)
{
  /* fake a mutex - we don't need it for LoadTerrainOverview() */
  SharedMutex mutex;

  TerrainLoader loader(mutex, raster_tile_cache, true, all, env);
  return loader.LoadOverview(dir, path, world_file);
}

inline bool
TerrainLoader::UpdateTiles(struct zzip_dir *dir, const char *path,
                           int x, int y, unsigned radius)
{
  assert(!scan_overview);

  if (!raster_tile_cache.PollTiles(x, y, radius))
    /* nothing to do */
    return true;

  bool success = LoadJPG2000(dir, path);
  raster_tile_cache.FinishTileUpdate();
  return success;
}

bool
UpdateTerrainTiles(struct zzip_dir *dir, const char *path,
                   RasterTileCache &raster_tile_cache, SharedMutex &mutex,
                   int x, int y, unsigned radius)
{
  if (!raster_tile_cache.IsValid())
    return false;

  NullOperationEnvironment env;
  TerrainLoader loader(mutex, raster_tile_cache, false, true, env);
  return loader.UpdateTiles(dir, path, x, y, radius);
}

bool
UpdateTerrainTiles(struct zzip_dir *dir, const char *path,
                   RasterTileCache &raster_tile_cache, SharedMutex &mutex,
                   const RasterProjection &projection,
                   const GeoPoint &location, double radius)
{
  const auto raster_location = projection.ProjectCoarse(location);

  return UpdateTerrainTiles(dir, path, raster_tile_cache, mutex,
                            raster_location.x, raster_location.y,
                            projection.DistancePixelsCoarse(radius));
}
