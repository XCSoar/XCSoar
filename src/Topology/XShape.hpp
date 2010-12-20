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

#ifndef TOPOLOGY_XSHAPE_HPP
#define TOPOLOGY_XSHAPE_HPP

#include "Util/NonCopyable.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Geo/GeoBounds.hpp"
#include "shapelib/mapshape.h"

#include <tchar.h>
#include <assert.h>

class XShape : private NonCopyable {
  enum {
    MAX_LINES = 32,
  };

  GeoBounds bounds;

  int type;

  /**
   * The number of elements in the "lines" array.
   */
  unsigned num_lines;

  /**
   * An array which stores the number of points of each line.  This is
   * a fixed-size array to reduce the number of allocations at
   * runtime.
   */
  unsigned short lines[MAX_LINES];

  /**
   * All points of all lines.
   */
  GeoPoint *points;

  TCHAR *label;

public:
  XShape(shapefileObj *shpfile, int i, int label_field=-1);
  ~XShape();

  bool is_visible(int label_field) const {
    return label_field < 0 || label != NULL;
  }

  const GeoBounds &get_bounds() const {
    return bounds;
  }

  int get_type() const {
    return type;
  }

  unsigned get_number_of_lines() const {
    return num_lines;
  }

  const unsigned short *get_lines() const {
    return lines;
  }

  const GeoPoint *get_points() const {
    return points;
  }

  const TCHAR *get_label() const {
    return label;
  }
};

#endif
