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

#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "WindowProjection.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/LabelBlock.hpp"
#include "SettingsMap.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Screen/Layout.hpp"
#include "shapelib/mapserver.h"

#include <zzip/lib.h>

#include <algorithm>
#include <stdlib.h>
#include <tchar.h>
#include <ctype.h> // needed for Wine

TopographyFile::TopographyFile(struct zzip_dir *_dir, const char *filename,
                           fixed _threshold,
                           fixed _labelThreshold,
                           fixed _labelImportantThreshold,
                           const Color thecolor,
                           int _label_field, int _icon,
                           int _pen_width)
  :dir(_dir), label_field(_label_field), icon(_icon),
   pen_width(_pen_width),
   color(thecolor), scaleThreshold(_threshold),
   labelThreshold(_labelThreshold),
   labelImportantThreshold(_labelImportantThreshold),
   shapefileopen(false)
{
  if (msShapefileOpen(&shpfile, "rb", dir, filename, 0) == -1)
    return;

  shpCache.resize_discard(shpfile.numshapes);

  shapefileopen = true;

  if (dir != NULL)
    ++dir->refcount;

  cache_bounds.west = cache_bounds.east =
    cache_bounds.south = cache_bounds.north = Angle::zero();

  std::fill(shpCache.begin(), shpCache.end(), (XShape *)NULL);
}

TopographyFile::~TopographyFile()
{
  if (!shapefileopen)
    return;

  ClearCache();
  msShapefileClose(&shpfile);

  if (dir != NULL) {
    --dir->refcount;
    zzip_dir_free(dir);
  }
}

void
TopographyFile::ClearCache()
{
  for (unsigned i = 0; i < shpCache.size(); i++) {
    delete shpCache[i];
    shpCache[i] = NULL;
  }
}

rectObj
TopographyFile::ConvertRect(const GeoBounds &br)
{
  rectObj dest;
  dest.minx = br.west.value_degrees();
  dest.maxx = br.east.value_degrees();
  dest.miny = br.south.value_degrees();
  dest.maxy = br.north.value_degrees();
  return dest;
}

bool
TopographyFile::updateCache(const WindowProjection &map_projection)
{
  if (!shapefileopen)
    return false;

  if (map_projection.GetMapScale() > scaleThreshold)
    /* not visible, don't update cache now */
    return false;

  const GeoBounds screenRect =
    map_projection.GetScreenBounds();
  if (cache_bounds.inside(screenRect))
    /* the cache is still fresh */
    return false;

  cache_bounds = map_projection.GetScreenBounds().scale(fixed_two);

  rectObj deg_bounds = ConvertRect(cache_bounds);

  // Test which shapes are inside the given bounds and save the
  // status to shpfile.status
  msShapefileWhichShapes(&shpfile, dir, deg_bounds, 0);

  // If not a single shape is inside the bounds
  if (!shpfile.status) {
    // ... clear the whole buffer
    ClearCache();
    return false;
  }

  // Iterate through the shapefile entries
  for (int i = 0; i < shpfile.numshapes; i++) {
    if (!msGetBit(shpfile.status, i)) {
      // If the shape is outside the bounds
      // delete the shape from the cache
      delete shpCache[i];
      shpCache[i] = NULL;
    } else if (shpCache[i] == NULL) {
      // If the shape is inside the bounds and if the
      // shape isn't cached yet -> cache the shape
      shpCache[i] = new XShape(&shpfile, i, label_field);
    }
  }

  return true;
}

unsigned
TopographyFile::GetSkipSteps(fixed map_scale) const
{
  if (map_scale * 4 > scaleThreshold * 3)
    return 4;
  if (map_scale * 2 > scaleThreshold)
    return 3;
  if (map_scale * 4 > scaleThreshold)
    return 2;
  return 1;
}

#ifdef ENABLE_OPENGL

unsigned
TopographyFile::thinning_level(fixed map_scale) const
{
  if (map_scale * 2 > scaleThreshold)
    return 3;
  if (map_scale * 3 > scaleThreshold)
    return 2;
  if (map_scale * 4 > scaleThreshold)
    return 1;

  return 0;
}

unsigned
TopographyFile::min_point_distance(unsigned level) const
{
  switch (level) {
    case 1:
      return fixed(4) * scaleThreshold / Layout::Scale(30);
    case 2:
      return fixed(6) * scaleThreshold / Layout::Scale(30);
    case 3:
      return fixed(9) * scaleThreshold / Layout::Scale(30);
  }
  return 1;
}

#endif
