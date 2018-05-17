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

#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "Convert.hpp"
#include "Projection/WindowProjection.hpp"

#include <zzip/lib.h>

#include <algorithm>

TopographyFile::TopographyFile(zzip_dir *_dir, const char *filename,
                               double _threshold,
                               double _label_threshold,
                               double _important_label_threshold,
                               const Color _color,
                               int _label_field,
                               ResourceId _icon, ResourceId _big_icon,
                               unsigned _pen_width)
  :dir(_dir), first(nullptr),
   label_field(_label_field), icon(_icon), big_icon(_big_icon),
   pen_width(_pen_width),
   color(_color), scale_threshold(_threshold),
   label_threshold(_label_threshold),
   important_label_threshold(_important_label_threshold),
   cache_bounds(GeoBounds::Invalid())
{
  if (msShapefileOpen(&file, "rb", dir, filename, 0) == -1)
    return;

  if (file.numshapes == 0) {
    msShapefileClose(&file);
    return;
  }

  const auto file_bounds = ImportRect(file.bounds);
  if (!file_bounds.Check()) {
    /* malformed bounds */
    msShapefileClose(&file);
    return;
  }

  center = file_bounds.GetCenter();

  shapes.ResizeDiscard(file.numshapes);
  std::fill(shapes.begin(), shapes.end(), ShapeList(nullptr));

  if (dir != nullptr)
    ++dir->refcount;

  ++serial;
}

TopographyFile::~TopographyFile()
{
  if (IsEmpty())
    return;

  ClearCache();
  msShapefileClose(&file);

  if (dir != nullptr) {
    --dir->refcount;
    zzip_dir_free(dir);
  }
}

void
TopographyFile::ClearCache()
{
  for (auto i = shapes.begin(), end = shapes.end(); i != end; ++i) {
    delete i->shape;
    i->shape = nullptr;
  }

  first = nullptr;
}

static XShape *
LoadShape(shapefileObj *file, const GeoPoint &center, int i,
          int label_field)
{
  return new XShape(file, center, i, label_field);
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
  if (cache_bounds.IsValid() && cache_bounds.IsInside(screenRect))
    /* the cache is still fresh */
    return false;

  cache_bounds = screenRect.Scale(2);

  rectObj deg_bounds = ConvertRect(cache_bounds);

  // Test which shapes are inside the given bounds and save the
  // status to file.status
  switch (msShapefileWhichShapes(&file, dir, deg_bounds, 0)) {
  case MS_FAILURE:
    ClearCache();
    return false;

  case MS_DONE:
    /* screen is outside of map bounds */
    return false;

  case MS_SUCCESS:
    break;
  }

  assert(file.status != nullptr);

  // Iterate through the shapefile entries
  const ShapeList **current = &first;
  auto it = shapes.begin();
  for (int i = 0; i < file.numshapes; ++i, ++it) {
    if (!msGetBit(file.status, i)) {
      // If the shape is outside the bounds
      // delete the shape from the cache
      if (it->shape != nullptr) {
        assert(*current == it);

        /* remove from linked list (protected) */
        {
          const ScopeLock lock(mutex);
          *current = it->next;
          ++serial;
        }

        /* now it's unreachable, and we can delete the XShape without
           holding a lock */
        delete it->shape;
        it->shape = nullptr;
      }
    } else {
      // is inside the bounds
      if (it->shape == nullptr) {
        assert(*current != it);

        // shape isn't cached yet -> cache the shape
        it->shape = LoadShape(&file, center, i, label_field);
        it->next = *current;

        /* insert into linked list (protected) */
        {
          const ScopeLock lock(mutex);
          *current = it;
          ++serial;
        }
      }

      current = &it->next;
    }
  }
  // end of list marker
  assert(*current == nullptr);

  return true;
}

void
TopographyFile::LoadAll()
{
  // Iterate through the shapefile entries
  const ShapeList **current = &first;
  auto it = shapes.begin();
  for (int i = 0; i < file.numshapes; ++i, ++it) {
    if (it->shape == nullptr)
      // shape isn't cached yet -> cache the shape
      it->shape = LoadShape(&file, center, i, label_field);
    // update list pointer
    *current = it;
    current = &it->next;
  }
  // end of list marker
  *current = nullptr;

  ++serial;
}

unsigned
TopographyFile::GetSkipSteps(double map_scale) const
{
  if (map_scale > scale_threshold * 0.75)
    return 4;
  if (2 * map_scale > scale_threshold)
    return 3;
  if (4 * map_scale > scale_threshold)
    return 2;
  return 1;
}

#ifdef ENABLE_OPENGL

unsigned
TopographyFile::GetThinningLevel(double map_scale) const
{
  if (2 * map_scale > scale_threshold)
    return 3;
  if (map_scale * 3 > scale_threshold)
    return 2;
  if (4 * map_scale > scale_threshold)
    return 1;

  return 0;
}

unsigned
TopographyFile::GetMinimumPointDistance(unsigned level) const
{
  switch (level) {
    case 1:
      return (unsigned)(4 * scale_threshold / 30);
    case 2:
      return (unsigned)(6 * scale_threshold / 30);
    case 3:
      return (unsigned)(9 * scale_threshold / 30);
  }
  return 1;
}

#endif
