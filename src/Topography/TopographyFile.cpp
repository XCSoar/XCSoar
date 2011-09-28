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
#include "Projection/WindowProjection.hpp"
#include "Screen/Layout.hpp"

#include <zzip/lib.h>

#include <algorithm>
#include <stdlib.h>

TopographyFile::TopographyFile(struct zzip_dir *_dir, const char *filename,
                               fixed _threshold,
                               fixed _label_threshold,
                               fixed _important_label_threshold,
                               const Color thecolor,
                               int _label_field, int _icon,
                               int _pen_width)
  :dir(_dir), first(NULL),
   label_field(_label_field), icon(_icon),
   pen_width(_pen_width),
   color(thecolor), scale_threshold(_threshold),
   label_threshold(_label_threshold),
   important_label_threshold(_important_label_threshold)
{
  if (msShapefileOpen(&file, "rb", dir, filename, 0) == -1)
    return;

  if (file.numshapes == 0) {
    msShapefileClose(&file);
    return;
  }

  shapes.resize_discard(file.numshapes);
  std::fill(shapes.begin(), shapes.end(), ShapeList(NULL));

  if (dir != NULL)
    ++dir->refcount;

  cache_bounds.west = cache_bounds.east =
    cache_bounds.south = cache_bounds.north = Angle::Zero();
}

TopographyFile::~TopographyFile()
{
  if (IsEmpty())
    return;

  ClearCache();
  msShapefileClose(&file);

  if (dir != NULL) {
    --dir->refcount;
    zzip_dir_free(dir);
  }
}

void
TopographyFile::ClearCache()
{
  for (unsigned i = 0; i < shapes.size(); i++) {
    delete shapes[i].shape;
    shapes[i].shape = NULL;
  }

  first = NULL;
}

gcc_pure
static rectObj
ConvertRect(const GeoBounds &br)
{
  rectObj dest;
  dest.minx = br.west.Degrees();
  dest.maxx = br.east.Degrees();
  dest.miny = br.south.Degrees();
  dest.maxy = br.north.Degrees();
  return dest;
}

bool
TopographyFile::Update(const WindowProjection &map_projection)
{
  if (IsEmpty())
    return false;

  if (map_projection.GetMapScale() > scale_threshold)
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
  // status to file.status
  msShapefileWhichShapes(&file, dir, deg_bounds, 0);

  // If not a single shape is inside the bounds
  if (!file.status) {
    // ... clear the whole buffer
    ClearCache();
    return false;
  }

  // Iterate through the shapefile entries
  for (int i = 0; i < file.numshapes; i++) {
    if (!msGetBit(file.status, i)) {
      // If the shape is outside the bounds
      // delete the shape from the cache
      delete shapes[i].shape;
      shapes[i].shape = NULL;
    } else if (shapes[i].shape == NULL) {
      // If the shape is inside the bounds and if the
      // shape isn't cached yet -> cache the shape
      shapes[i].shape = new XShape(&file, i, label_field);
    }
  }

  ShapeList::NotNull not_null;
  XShapePointerArray::iterator end = shapes.end(), it = shapes.begin();
  it = std::find_if(it, end, not_null);
  if (it != shapes.end()) {
    ShapeList *current = &*it;
    first = current;

    while (true) {
      ++it;
      it = std::find_if(it, end, not_null);
      if (it == end) {
        current->next = NULL;
        break;
      }

      ShapeList *next = &*it;
      current->next = next;
      current = next;
    }
  } else
    first = NULL;

  return true;
}

unsigned
TopographyFile::GetSkipSteps(fixed map_scale) const
{
  if (map_scale * 4 > scale_threshold * 3)
    return 4;
  if (map_scale * 2 > scale_threshold)
    return 3;
  if (map_scale * 4 > scale_threshold)
    return 2;
  return 1;
}

#ifdef ENABLE_OPENGL

unsigned
TopographyFile::GetThinningLevel(fixed map_scale) const
{
  if (map_scale * 2 > scale_threshold)
    return 3;
  if (map_scale * 3 > scale_threshold)
    return 2;
  if (map_scale * 4 > scale_threshold)
    return 1;

  return 0;
}

unsigned
TopographyFile::GetMinimumPointDistance(unsigned level) const
{
  switch (level) {
    case 1:
      return fixed(4) * scale_threshold / Layout::Scale(30);
    case 2:
      return fixed(6) * scale_threshold / Layout::Scale(30);
    case 3:
      return fixed(9) * scale_threshold / Layout::Scale(30);
  }
  return 1;
}

#endif
