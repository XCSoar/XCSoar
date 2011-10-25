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

#ifndef TOPOGRAPHY_HPP
#define TOPOGRAPHY_HPP

#include "shapelib/mapserver.h"
#include "Geo/GeoBounds.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/AllocatedArray.hpp"
#include "Math/fixed.hpp"
#include "Screen/Color.hpp"

#include <assert.h>

struct GeoPoint;
class Canvas;
class WindowProjection;
struct GeoBounds;
class LabelBlock;
struct SETTINGS_MAP;
class XShape;
struct zzip_dir;

class TopographyFile : private NonCopyable {
  struct ShapeList {
    struct NotNull {
      bool operator()(const ShapeList &x) const {
        return x.shape != NULL;
      }
    };

    const ShapeList *next;

    const XShape *shape;

    ShapeList() {}
    ShapeList(const XShape *_shape):shape(_shape) {}
  };

  typedef AllocatedArray<ShapeList> XShapePointerArray;

  struct zzip_dir *dir;

  shapefileObj file;

  XShapePointerArray shapes;
  const ShapeList *first;

  int label_field, icon, pen_width;

  Color color;

  /**
   * The threshold value for the visibility check. If the current scale
   * is below this value the contents of this TopographyFile will be drawn.
   */
  fixed scale_threshold;

  /**
   * The threshold value for label rendering. If the current scale
   * is below this value no labels of this TopographyFile will be drawn.
   */
  fixed label_threshold;

  /**
   * The threshold value for label rendering in important style . If the current
   * scale is below this value labels of this TopographyFile will be drawn
   * in standard style
   */
  fixed important_label_threshold;

  /**
   * The current scope of the shape cache.  If the screen exceeds this
   * rectangle, then we need to update the cache.
   */
  GeoBounds cache_bounds;

public:
  class const_iterator {
    friend class TopographyFile;

    const ShapeList *current;

    const_iterator(const ShapeList *p):current(p) {}

  public:
    const_iterator &operator++() {
      assert(current != NULL);

      current = current->next;
      return *this;
    }

    const XShape &operator*() const {
      assert(current != NULL);
      assert(current->shape != NULL);

      return *current->shape;
    }

    const XShape *operator->() const {
      assert(current != NULL);

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
   * @param thecolor The color to use for drawing
   * @param label_field The field in which the labels should be searched
   * @param icon the resource id of the icon, 0 for no icon
   * @param pen_width The pen width used for line drawing
   * @param label_threshold the zoom threshold for label rendering
   * @param important_label_threshold labels below this zoom threshold will
   * be renderd in default style
   * @return
   */
  TopographyFile(struct zzip_dir *dir, const char *shpname,
                 fixed threshold, fixed label_threshold,
                 fixed important_label_threshold,
                 const Color color,
                 int label_field=-1, int icon=0,
                 int pen_width=1);

  /**
   * The destructor clears the cache and closes the shapefile
   */
  ~TopographyFile();

  bool IsEmpty() const {
    return shapes.Size() == 0;
  }

  bool IsVisible(fixed map_scale) const {
    return map_scale <= scale_threshold;
  }

  bool IsLabelVisible(fixed map_scale) const {
    return map_scale <= label_threshold;
  }

  bool IsLabelImportant(fixed map_scale) const {
    return map_scale <= important_label_threshold;
  }

  int GetIcon() const {
    return icon;
  }

  Color GetColor() const {
    return color;
  }

  int GetPenWidth() const {
    return pen_width;
  }

  const_iterator begin() const {
    return const_iterator(first);
  }

  const_iterator end() const {
    return const_iterator(NULL);
  }

  gcc_pure
  unsigned GetSkipSteps(fixed map_scale) const;

#ifdef ENABLE_OPENGL
  /**
   * @return thinning level, range: 0 .. XShape::THINNING_LEVELS-1
   */
  gcc_pure
  unsigned GetThinningLevel(fixed map_scale) const;

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

protected:
  void ClearCache();
};

#endif
