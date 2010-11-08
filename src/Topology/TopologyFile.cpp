/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Topology/TopologyFile.hpp"
#include "Topology/XShape.hpp"
#include "Screen/Util.hpp"
#include "WindowProjection.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/LabelBlock.hpp"
#include "SettingsMap.hpp"
#include "Navigation/GeoPoint.hpp"
#include "shapelib/map.h"

#include <algorithm>
#include <stdlib.h>
#include <tchar.h>
#include <ctype.h> // needed for Wine

TopologyFile::TopologyFile(const char *filename, fixed _threshold,
                           const Color thecolor,
                           int _label_field, int _icon)
  :label_field(_label_field), icon(_icon),
   color(thecolor),
   scaleThreshold(_threshold),
  shapefileopen(false)
{
  if (msSHPOpenFile(&shpfile, "rb", filename) == -1)
    return;

  shpCache.resize_discard(shpfile.numshapes);

  shapefileopen = true;

  cache_bounds.west = cache_bounds.east =
    cache_bounds.south = cache_bounds.north = Angle::native(fixed_zero);

  std::fill(shpCache.begin(), shpCache.end(), (XShape *)NULL);
}

TopologyFile::~TopologyFile()
{
  if (!shapefileopen)
    return;

  ClearCache();
  msSHPCloseFile(&shpfile);
}

void
TopologyFile::ClearCache()
{
  for (unsigned i = 0; i < shpCache.size(); i++) {
    delete shpCache[i];
    shpCache[i] = NULL;
  }
}

rectObj
TopologyFile::ConvertRect(const GeoBounds &br)
{
  rectObj dest;
  dest.minx = br.west.value_degrees();
  dest.maxx = br.east.value_degrees();
  dest.miny = br.south.value_degrees();
  dest.maxy = br.north.value_degrees();
  return dest;
}

void
TopologyFile::updateCache(const WindowProjection &map_projection)
{
  if (!shapefileopen)
    return;

  if (map_projection.GetMapScale() > scaleThreshold)
    /* not visible, don't update cache now */
    return;

  const GeoBounds screenRect =
    map_projection.GetScreenBounds();
  if (cache_bounds.inside(screenRect))
    /* the cache is still fresh */
    return;

  cache_bounds = map_projection.GetScreenBounds().scale(fixed_two);

  rectObj deg_bounds = ConvertRect(cache_bounds);

  // Test which shapes are inside the given bounds and save the
  // status to shpfile.status
  msSHPWhichShapes(&shpfile, deg_bounds, 0);

  // If not a single shape is inside the bounds
  if (!shpfile.status) {
    // ... clear the whole buffer
    ClearCache();
    return;
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
}

unsigned
TopologyFile::GetSkipSteps(fixed map_scale) const
{
  if (map_scale * 4 > scaleThreshold * 3)
    return 4;
  if (map_scale * 2 > scaleThreshold)
    return 3;
  if (map_scale * 4 > scaleThreshold)
    return 2;

  return 1;
}
