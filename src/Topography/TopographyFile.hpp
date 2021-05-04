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

#ifndef TOPOGRAPHY_HPP
#define TOPOGRAPHY_HPP

#include "shapelib/mapserver.h"
#include "Geo/GeoBounds.hpp"
#include "util/AllocatedArray.hxx"
#include "util/IntrusiveForwardList.hxx"
#include "util/Serial.hpp"
#include "ui/canvas/PortableColor.hpp"
#include "ResourceId.hpp"
#include "thread/Mutex.hxx"

#ifdef ENABLE_OPENGL
#include "XShapePoint.hpp"
#endif

#include <cassert>
#include <memory>

class WindowProjection;
class XShape;
struct zzip_dir;

/**
 * C++ wrapper for #shapefileObj;
 */
class ShapeFile {
  shapefileObj obj;

public:
  /**
   * Throws on error.
   */
  ShapeFile(zzip_dir *dir, const char *filename);

  ~ShapeFile() noexcept {
    msShapefileClose(&obj);
  }

  ShapeFile(const ShapeFile &) = delete;
  ShapeFile &operator=(const ShapeFile &) = delete;

  std::size_t size() const noexcept {
    return obj.numshapes;
  }

  const auto &GetBounds() const noexcept {
    return obj.bounds;
  }

  int WhichShapes(struct zzip_dir *dir, rectObj rect) noexcept {
    return msShapefileWhichShapes(&obj, dir, rect, 0);
  }

  ms_const_bitarray GetStatus() const noexcept {
    return obj.status;
  }

  /**
   * Throws on error.
   */
  void ReadShape(shapeObj &shape, std::size_t i);

  [[gnu::pure]]
  const char *ReadLabel(std::size_t i, unsigned field) noexcept;
};

class TopographyFile {
  struct ShapeEnvelope final : IntrusiveForwardListHook {
    std::unique_ptr<const XShape> shape;
  };

  /**
   * This gets incremented by Update().
   */
  Serial serial;

  zzip_dir *const dir;

  ShapeFile file;

  /**
   * The center of shapefileObj::bounds.
   */
  GeoPoint center;

  AllocatedArray<ShapeEnvelope> shapes;

  using ShapeList = IntrusiveForwardList<ShapeEnvelope>;
  ShapeList list;

  const int label_field;

  const ResourceId icon, big_icon;

  const unsigned pen_width;

  const BGRA8Color color;

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
  GeoBounds cache_bounds = GeoBounds::Invalid();

public:
  /**
   * Protects #serial, #shapes, #first.
   * The caller is responsible for locking it.
   */
  mutable Mutex mutex;

  class const_iterator {
    friend class TopographyFile;

    ShapeList::const_iterator i;

    constexpr const_iterator(ShapeList::const_iterator _i) noexcept:i(_i) {}

  public:
    const_iterator &operator++() {
      ++i;
      return *this;
    }

    const XShape &operator*() const {
      return *i->shape;
    }

    const XShape *operator->() const {
      return i->shape.operator->();
    }

    bool operator==(const const_iterator &other) const {
      return i == other.i;
    }

    bool operator!=(const const_iterator &other) const {
      return !(*this == other);
    }
  };

public:
  /**
   * The constructor opens the given shapefile and clears the cache
   *
   * Throws on error.
   *
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
   */
  TopographyFile(zzip_dir *dir, const char *shpname,
                 double threshold, double label_threshold,
                 double important_label_threshold,
                 const BGRA8Color color,
                 int label_field=-1,
                 ResourceId icon=ResourceId::Null(),
                 ResourceId big_icon=ResourceId::Null(),
                 unsigned pen_width=1);

  TopographyFile(const TopographyFile &) = delete;

  /**
   * The destructor clears the cache and closes the shapefile
   */
  ~TopographyFile() noexcept;

  const Serial &GetSerial() const noexcept {
    return serial;
  }

  const GeoPoint &GetCenter() const noexcept {
    return center;
  }

  bool IsVisible(double map_scale) const noexcept {
    return map_scale <= scale_threshold;
  }

  bool IsLabelVisible(double map_scale) const noexcept {
    return map_scale <= label_threshold;
  }

  /**
   * Returns the map scale threshold that will be reached next by
   * zooming in.  This is used to decide when to rescan shapes that
   * must be loaded.  A negative value is returned when all thresholds
   * have been reached already.
   */
  [[gnu::pure]]
  double GetNextScaleThreshold(double map_scale) const noexcept {
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

  bool IsLabelImportant(double map_scale) const noexcept {
    return map_scale <= important_label_threshold;
  }

  ResourceId GetIcon() const noexcept {
    return icon;
  }

  ResourceId GetBigIcon() const noexcept {
    return big_icon;
  }

  const auto &GetColor() const noexcept {
    return color;
  }

  unsigned GetPenWidth() const noexcept {
    return pen_width;
  }

  const_iterator begin() const noexcept {
    return const_iterator{list.begin()};
  }

  const_iterator end() const noexcept {
    return const_iterator{list.end()};
  }

  [[gnu::pure]]
  unsigned GetSkipSteps(double map_scale) const noexcept;

#ifdef ENABLE_OPENGL
  [[gnu::pure]]
  GeoPoint ToGeoPoint(const ShapePoint &p) const noexcept {
    return GeoPoint(center.longitude + Angle::Native(p.x),
                    center.latitude + Angle::Native(p.y));
  }

  /**
   * @return thinning level, range: 0 .. XShape::THINNING_LEVELS-1
   */
  [[gnu::pure]]
  unsigned GetThinningLevel(double map_scale) const noexcept;

  /**
   * @return minimum distance between points in ShapePoint coordinates
   */
  [[gnu::pure]]
  unsigned GetMinimumPointDistance(unsigned level) const noexcept;
#endif

  /**
   * Throws on error.
   *
   * @return true if new data from the topography file has been loaded
   */
  bool Update(const WindowProjection &map_projection);

  /**
   * Throws on error.
   *
   * Load all shapes into memory.  For debugging purposes.
   */
  void LoadAll();

protected:
  void ClearCache() noexcept;
};

#endif
