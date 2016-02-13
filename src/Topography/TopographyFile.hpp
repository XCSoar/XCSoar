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

#ifndef TOPOGRAPHY_HPP
#define TOPOGRAPHY_HPP

#include "shapelib/mapserver.h"
#include "Geo/GeoBounds.hpp"
#include "Util/AllocatedArray.hxx"
#include "Util/Serial.hpp"
#include "Screen/Color.hpp"
#include "ResourceId.hpp"
#include "Thread/Mutex.hpp"

#ifdef ENABLE_OPENGL
#include "XShapePoint.hpp"
#endif

#include <assert.h>

class WindowProjection;
class XShape;
struct zzip_dir;

class TopographyFile {
  struct ShapeList {
    const ShapeList *next;

    const XShape *shape;

    ShapeList() {}
    ShapeList(const XShape *_shape):shape(_shape) {}
  };

  /**
   * This gets incremented by Update().
   */
  Serial serial;

  zzip_dir *const dir;

  shapefileObj file;

  /**
   * The center of shapefileObj::bounds.
   */
  GeoPoint center;

  AllocatedArray<ShapeList> shapes;
  const ShapeList *first;

  const int label_field;

  const ResourceId icon, big_icon;

  const unsigned pen_width;

  const Color color;

  /**
   * The threshold value for the visibility check. If the current scale
   * is below this value the contents of this TopographyFile will be drawn.
   */
  const double scale_threshold;

  /**
   * The threshold value for label rendering. If the current scale
   * is below this value no labels of this TopographyFile will be drawn.
   */
  const double label_threshold;

  /**
   * The threshold value for label rendering in important style . If the current
   * scale is below this value labels of this TopographyFile will be drawn
   * in standard style
   */
  const double important_label_threshold;

  /**
   * The current scope of the shape cache.  If the screen exceeds this
   * rectangle, then we need to update the cache.
   */
  GeoBounds cache_bounds;

public:
  /**
   * Protects #serial, #shapes, #first.
   * The caller is responsible for locking it.
   */
  mutable Mutex mutex;

  class const_iterator {
    friend class TopographyFile;

    const ShapeList *current;

    const_iterator(const ShapeList *p):current(p) {}

  public:
    const_iterator &operator++() {
      assert(current != nullptr);

      current = current->next;
      return *this;
    }

    const XShape &operator*() const {
      assert(current != nullptr);
      assert(current->shape != nullptr);

      return *current->shape;
    }

    const XShape *operator->() const {
      assert(current != nullptr);

      return current->shape;
    }

    bool operator==(const const_iterator &other) const {
      return current == other.current;
    }

    bool operator!=(const const_iterator &other) const {
      return !(*this == other);
    }
  };

public:
  /**
   * The constructor opens the given shapefile and clears the cache
   * @param shpname The shapefile to open (*.shp)
   * @param threshold the zoom threshold for displaying this object
   * @param color The color to use for drawing, including alpha for OpenGL
   * @param label_field The field in which the labels should be searched
   * @param icon the resource id of the icon, 0 for no icon
   * @param big_icon the resource id of the big icon, 0 for no big icon
   * @param pen_width The pen width used for line drawing
   * @param label_threshold the zoom threshold for label rendering
   * @param important_label_threshold labels below this zoom threshold will
   * be rendered in default style
   * @return
   */
  TopographyFile(zzip_dir *dir, const char *shpname,
                 double threshold, double label_threshold,
                 double important_label_threshold,
                 const Color color,
                 int label_field=-1,
                 ResourceId icon=ResourceId::Null(),
                 ResourceId big_icon=ResourceId::Null(),
                 unsigned pen_width=1);

  TopographyFile(const TopographyFile &) = delete;

  /**
   * The destructor clears the cache and closes the shapefile
   */
  ~TopographyFile();

  const Serial &GetSerial() const {
    assert(mutex.IsLockedByCurrent());

    return serial;
  }

  const GeoPoint &GetCenter() const {
    return center;
  }

  bool IsEmpty() const {
    return shapes.empty();
  }

  bool IsVisible(double map_scale) const {
    return map_scale <= scale_threshold;
  }

  bool IsLabelVisible(double map_scale) const {
    return map_scale <= label_threshold;
  }

  /**
   * Returns the map scale threshold that will be reached next by
   * zooming in.  This is used to decide when to rescan shapes that
   * must be loaded.  A negative value is returned when all thresholds
   * have been reached already.
   */
  gcc_pure
  double GetNextScaleThreshold(double map_scale) const {
    return map_scale <= scale_threshold
      ? (map_scale <= label_threshold
         /* both thresholds reached: not relevant */
         ? -1.
         /* only label_threshold not yet reached */
         : label_threshold)
      /* scale_threshold not yet reached */
      : (map_scale <= label_threshold
         /* only scale_threshold not yet reached */
         ? scale_threshold
         /* choose the bigger threshold, that will trigger next */
         : std::max(scale_threshold, label_threshold));
  }

  bool IsLabelImportant(double map_scale) const {
    return map_scale <= important_label_threshold;
  }

  ResourceId GetIcon() const {
    return icon;
  }

  ResourceId GetBigIcon() const {
    return big_icon;
  }

  Color GetColor() const {
    return color;
  }

  unsigned GetPenWidth() const {
    return pen_width;
  }

  const_iterator begin() const {
    assert(mutex.IsLockedByCurrent());

    return const_iterator(first);
  }

  const_iterator end() const {
    assert(mutex.IsLockedByCurrent());

    return const_iterator(nullptr);
  }

  gcc_pure
  unsigned GetSkipSteps(double map_scale) const;

#ifdef ENABLE_OPENGL
  gcc_pure
  GeoPoint ToGeoPoint(const ShapePoint &p) const {
    return GeoPoint(center.longitude + Angle::Native(p.x),
                    center.latitude + Angle::Native(p.y));
  }

  /**
   * @return thinning level, range: 0 .. XShape::THINNING_LEVELS-1
   */
  gcc_pure
  unsigned GetThinningLevel(double map_scale) const;

  /**
   * @return minimum distance between points in ShapePoint coordinates
   */
  gcc_pure
  unsigned GetMinimumPointDistance(unsigned level) const;
#endif

  /**
   * @return true if new data from the topography file has been loaded
   */
  bool Update(const WindowProjection &map_projection);

  /**
   * Load all shapes into memory.  For debugging purposes.
   */
  void LoadAll();

protected:
  void ClearCache();
};

#endif
