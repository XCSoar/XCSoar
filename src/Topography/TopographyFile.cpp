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

#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "Convert.hpp"
#include "Projection/WindowProjection.hpp"
#include "util/ScopeExit.hxx"

#include <zzip/lib.h>

#include <algorithm>
#include <stdexcept>

inline
ShapeFile::ShapeFile(zzip_dir *dir, const char *filename)
{
  if (msShapefileOpen(&obj, "rb", dir, filename, 0) == -1)
    throw std::runtime_error{"Failed to open shapefile"};
}

inline void
ShapeFile::ReadShape(shapeObj &shape, std::size_t i)
{
  msSHPReadShape(obj.hSHP, i, &shape);
  if (shape.type == MS_SHAPE_NULL)
    throw std::runtime_error{"Failed to read shape"};
}

inline const char *
ShapeFile::ReadLabel(std::size_t i, unsigned field) noexcept
{
  return msDBFReadStringAttribute(obj.hDBF, i, field);
}

TopographyFile::TopographyFile(zzip_dir *_dir, const char *filename,
                               double _threshold,
                               double _label_threshold,
                               double _important_label_threshold,
                               const BGRA8Color _color,
                               int _label_field,
                               ResourceId _icon, ResourceId _big_icon,
                               unsigned _pen_width)
  :dir(_dir),
   file(dir, filename),
   label_field(_label_field), icon(_icon), big_icon(_big_icon),
   pen_width(_pen_width),
   color(_color), scale_threshold(_threshold),
   label_threshold(_label_threshold),
   important_label_threshold(_important_label_threshold)
{
  const std::size_t n_shapes = file.size();
  constexpr std::size_t MAX_SHAPES = 16 * 1024 * 1024;
  if (n_shapes == 0)
    throw std::runtime_error{"Empty shapefile"};

  if (n_shapes > MAX_SHAPES)
    throw std::runtime_error{"Too many shapes in shapefile"};

  const auto file_bounds = ImportRect(file.GetBounds());
  if (!file_bounds.Check())
    throw std::runtime_error{"Malformed shapefile bounds"};

  center = file_bounds.GetCenter();

  shapes.ResizeDiscard(n_shapes);

  if (dir != nullptr)
    ++dir->refcount;

  ++serial;
}

TopographyFile::~TopographyFile() noexcept
{
  if (dir != nullptr) {
    --dir->refcount;
    zzip_dir_free(dir);
  }
}

void
TopographyFile::ClearCache() noexcept
{
  for (auto &i : shapes)
    i.shape.reset();

  list.clear();
}

static std::unique_ptr<XShape>
LoadShape(ShapeFile &file, GeoPoint &center, std::size_t i, int label_field)
{
  shapeObj shape;
  msInitShape(&shape);
  AtScopeExit(&shape) { msFreeShape(&shape); };
  file.ReadShape(shape, i);

  const char *label = label_field >= 0
    ? file.ReadLabel(i, label_field)
    : nullptr;

  return std::make_unique<XShape>(shape, center, label);
}

bool
TopographyFile::Update(const WindowProjection &map_projection)
{
  if (map_projection.GetMapScale() > scale_threshold)
    /* not visible, don't update cache now */
    return false;

  const GeoBounds screenRect =
    map_projection.GetScreenBounds();
  if (cache_bounds.IsValid() && cache_bounds.IsInside(screenRect))
    /* the cache is still fresh */
    return false;

  cache_bounds = screenRect.Scale(2);

  // Test which shapes are inside the given bounds and save the
  // status to file.status
  switch (file.WhichShapes(dir, ConvertRect(cache_bounds))) {
  case MS_FAILURE:
    ClearCache();
    throw std::runtime_error{"Failed to update shapefile"};

  case MS_DONE:
    /* screen is outside of map bounds */
    return false;

  case MS_SUCCESS:
    break;
  }

  const auto status = file.GetStatus();
  assert(status != nullptr);

  // Iterate through the shapefile entries
  auto prev = list.before_begin();
  auto it = shapes.begin();
  for (std::size_t i = 0; i < file.size(); ++i, ++it) {
    if (!msGetBit(status, i)) {
      // If the shape is outside the bounds
      // delete the shape from the cache
      if (it->shape != nullptr) {
        assert(&*std::next(prev) == it);

        /* remove from linked list (protected) */
        {
          const std::lock_guard<Mutex> lock(mutex);
          list.erase_after(prev);
          ++serial;
        }

        /* now it's unreachable, and we can delete the XShape without
           holding a lock */
        it->shape.reset();
      }
    } else {
      // is inside the bounds
      if (it->shape == nullptr) {
        assert(&*std::next(prev) != it);

        // shape isn't cached yet -> cache the shape
        it->shape = LoadShape(file, center, i, label_field);

        /* insert into linked list (protected) */
        {
          const std::lock_guard<Mutex> lock(mutex);
          prev = list.insert_after(prev, *it);
          ++serial;
        }
      } else {
        ++prev;
        assert(&*prev == it);
      }
    }
  }

  assert(std::next(prev) == list.end());

  return true;
}

void
TopographyFile::LoadAll()
{
  // Iterate through the shapefile entries
  auto prev = list.before_begin();
  auto it = shapes.begin();
  for (std::size_t i = 0; i < file.size(); ++i, ++it) {
    if (it->shape == nullptr) {
      assert(&*std::next(prev) != it);
      // shape isn't cached yet -> cache the shape
      it->shape = LoadShape(file, center, i, label_field);
      // update list pointer
      prev = list.insert_after(prev, *it);
    } else {
      ++prev;
      assert(&*prev == it);
    }
  }

  assert(std::next(prev) == list.end());

  ++serial;
}

unsigned
TopographyFile::GetSkipSteps(double map_scale) const noexcept
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
TopographyFile::GetThinningLevel(double map_scale) const noexcept
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
TopographyFile::GetMinimumPointDistance(unsigned level) const noexcept
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
