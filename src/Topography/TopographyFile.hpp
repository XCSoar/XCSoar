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
#include "Screen/Fonts.hpp"

struct GeoPoint;
class Canvas;
class WindowProjection;
struct GeoBounds;
class LabelBlock;
struct SETTINGS_MAP;
class XShape;
struct zzip_dir;

class TopographyFile : private NonCopyable {
  struct zzip_dir *dir;

  shapefileObj shpfile;

  AllocatedArray<XShape *> shpCache;

  int label_field, icon, pen_width;

  Color color;

  /**
   * The threshold value for the visibility check. If the current scale
   * is below this value the contents of this TopographyFile will be drawn.
   */
  fixed scaleThreshold;

  /**
   * The threshold value for label rendering. If the current scale
   * is below this value no labels of this TopographyFile will be drawn.
   */
  fixed labelThreshold;

  /**
   * The threshold value for label rendering in important style . If the current
   * scale is below this value labels of this TopographyFile will be drawn
   * in standard style
   */
  fixed labelImportantThreshold;

  bool shapefileopen;

public:
  /**
   * The constructor opens the given shapefile and clears the cache
   * @param shpname The shapefile to open (*.shp)
   * @param threshold the zoom threshold for displaying this object
   * @param thecolor The color to use for drawing
   * @param label_field The field in which the labels should be searched
   * @param icon the resource id of the icon, 0 for no icon
   * @param pen_width The pen width used for line drawing
   * @param labelThreshold the zoom threshold for label rendering
   * @param labelImportantThreshold labels below this zoom threshold will
   * be renderd in default style
   * @return
   */
  TopographyFile(struct zzip_dir *dir, const char *shpname,
               fixed threshold, fixed labelThreshold,
               fixed labelImportantThreshold,
               const Color color,
               int label_field=-1, int icon=0,
               int pen_width=1);

  /**
   * The destructor clears the cache and closes the shapefile
   */
  ~TopographyFile();

  bool is_visible(fixed map_scale) const {
    return map_scale <= scaleThreshold;
  }

  bool is_label_visible(fixed map_scale) const {
    return map_scale <= labelThreshold;
  }

  bool is_label_important(fixed map_scale) const {
    return map_scale <= labelImportantThreshold;
  }

  int get_label_field() const {
    return label_field;
  }

  int get_icon() const {
    return icon;
  }

  Color get_color() const {
    return color;
  }

  int get_pen_width() const {
    return pen_width;
  }

  bool empty() const {
    return shpCache.size() == 0;
  }

  unsigned size() const {
    return shpCache.size();
  }

  XShape *operator[](unsigned i) const {
    return shpCache[i];
  }

  gcc_pure
  unsigned GetSkipSteps(fixed map_scale) const;

#ifdef ENABLE_OPENGL
  /**
   * @return thinning level, range: 0 .. XShape::THINNING_LEVELS-1
   */
  unsigned thinning_level(fixed map_scale) const;

  /**
   * @return minimum distance between points in ShapePoint coordinates
   */
  unsigned min_point_distance(unsigned level) const;
#endif

  /**
   * @return true if new data from the topography file has been loaded
   */
  bool updateCache(const WindowProjection &map_projection);

private:
  /**
   * The current scope of the shape cache.  If the screen exceeds this
   * rectangle, then we need to update the cache.
   */
  GeoBounds cache_bounds;

  static rectObj ConvertRect(const GeoBounds &br);

protected:
  void ClearCache();
};

#endif
