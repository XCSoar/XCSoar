/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "jasper/RasterTile.hpp"
#include "jasper/jasper.h"
#include "Math/Angle.hpp"

#include <algorithm>

using std::min;
using std::max;

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
RasterTile::GetField(unsigned lx, unsigned ly, unsigned ix, unsigned iy) const
{
  // we want to exit out of this function as soon as possible
  // if we have the wrong tile

  if (IsDisabled())
    return TERRAIN_INVALID;

  // check x in range
  if ((lx -= xstart) >= width)
    return TERRAIN_INVALID;

  // check y in range
  if ((ly -= ystart) >= height)
    return TERRAIN_INVALID;

  return buffer.get_interpolated(lx, ly, ix, iy);
}

bool
RasterTile::CheckTileVisibility(const int view_x, const int view_y)
{
  if (!width || !height) {
    Disable();
    return false;
  }

  const unsigned int dx1 = abs(view_x - xstart);
  const unsigned int dx2 = abs(xend - view_x);
  const unsigned int dy1 = abs(view_y - ystart);
  const unsigned int dy2 = abs(yend - view_y);

  if (min(dx1, dx2) * 2 < width * 3) {
    if (min(dy1, dy2) < height)
      return true;
  }
  if (min(dy1, dy2) * 2 < height * 3) {
    if (min(dx1, dx2) < width)
      return true;
  }
  if (IsEnabled()) {
    if ((max(dx1, dx2) > width * 2) || (max(dy1, dy2) > height * 2))
      Disable();
  }
  return false;
}

bool
RasterTile::VisibilityChanged(int view_x, int view_y)
{
  request = CheckTileVisibility(view_x, view_y) && IsDisabled();
  // JMW note: order of these is important!
  return request;
}

short*
RasterTileCache::GetImageBuffer(int index)
{
  if (index < MAX_RTC_TILES)
    return tiles[index].GetImageBuffer();

  return NULL;
}

const short *
RasterTileCache::GetImageBuffer(int index) const
{
  if (index< MAX_RTC_TILES)
    return tiles[index].GetImageBuffer();

  return NULL;
}

void
RasterTileCache::SetTile(int index, int xstart, int ystart, int xend, int yend)
{
  if (index >= MAX_RTC_TILES)
    return;

  RasterTile &tile = tiles[index];
  tile.xstart = xstart;
  tile.ystart = ystart;
  tile.xend = xend;
  tile.yend = yend;
  tile.width = xend - xstart;
  tile.height = yend - ystart;
}

bool
RasterTileCache::PollTiles(int x, int y)
{
  bool retval = false;
  int i, num_used = 0;

  if (scan_overview)
    return false;

  std::fill(ActiveTiles, ActiveTiles + MAX_ACTIVE_TILES, -1);

  for (i = MAX_RTC_TILES - 1; --i >= 0;) {
    if (tiles[i].VisibilityChanged(x, y))
      retval = true;

    if (tiles[i].IsEnabled()) {
      ActiveTiles[num_used] = i;
      num_used++;
    }
  }

  return retval;
}

bool
RasterTileCache::TileRequest(int index)
{
  int num_used = 0;

  if (index >= MAX_RTC_TILES) {
    // tile index too big!
    return false;
  }

  if (!tiles[index].request)
    return false;

  for (int i = 0; i < MAX_RTC_TILES; ++i)
    if (tiles[i].IsEnabled())
      num_used++;

  if (num_used < MAX_ACTIVE_TILES) {
    tiles[index].Enable();
    return true; // want to load this one!
  }

  return false; // not enough memory for it or not visible anyway
}

short
RasterTileCache::GetField(unsigned int lx, unsigned int ly) const
{
  if ((lx >= overview_width_fine) || (ly >= overview_height_fine))
    // outside overall bounds
    return RasterTile::TERRAIN_INVALID;

  unsigned px = lx, py = ly;
  const unsigned int ix = CombinedDivAndMod(px);
  const unsigned int iy = CombinedDivAndMod(py);

  short retval;

  // search starting from last found tile
  retval = tiles[tile_last].GetField(px, py, ix, iy);
  if (retval != RasterTile::TERRAIN_INVALID)
    return retval;

  int tile_this;
  for (int i = MAX_ACTIVE_TILES - 1; --i >= 0;) {
    if (((tile_this = ActiveTiles[i]) >= 0)
        && (tile_this != tile_last)
        && (retval = tiles[tile_this].GetField(px, py, ix, iy)) != RasterTile::TERRAIN_INVALID) {
      tile_last = tile_this;
      return retval;
    }
  }
  // still not found, so go to overview
  return Overview.get_interpolated(lx / RTC_SUBSAMPLING, ly / RTC_SUBSAMPLING);
}

void
RasterTileCache::SetSize(int _width, int _height)
{
  width = _width;
  height = _height;

  Overview.resize(width / RTC_SUBSAMPLING, height / RTC_SUBSAMPLING);
  overview_width_fine = width * 256;
  overview_height_fine = height * 256;
}

void
RasterTileCache::SetLatLonBounds(double _lon_min, double _lon_max,
                                 double _lat_min, double _lat_max)
{
  lat_min = min(_lat_min, _lat_max);
  lat_max = max(_lat_min, _lat_max);
  lon_min = min(_lon_min, _lon_max);
  lon_max = max(_lon_min, _lon_max);
}

void
RasterTileCache::Reset()
{
  tile_last = 0;
  width = 0;
  height = 0;
  initialised = false;
  scan_overview = true;

  Overview.reset();

  int i;
  for (i = 0; i < MAX_RTC_TILES; i++)
    tiles[i].Disable();

  std::fill(ActiveTiles, ActiveTiles + MAX_ACTIVE_TILES, -1);
}

void
RasterTileCache::SetInitialised(bool val)
{
  if (!initialised && val) {
    if (lon_max - lon_min < 0)
      return;

    if (lat_max - lat_min < 0)
      return;

    initialised = true;
    scan_overview = false;

    return;
  }
  initialised = val;
}

extern RasterTileCache *raster_tile_current;

void
RasterTileCache::LoadJPG2000(const char *jp2_filename)
{
  jas_stream_t *in;

  raster_tile_current = this;

  in = jas_stream_fopen(jp2_filename, "rb");
  if (!in) {
    SetInitialised(false);
  } else {
    jas_image_decode(in, -1, "xcsoar=1");
    jas_stream_close(in);
  }
}
