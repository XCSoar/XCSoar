/* Copyright_License {

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

#ifndef FLAT_TRIANGLE_FAN_HPP
#define FLAT_TRIANGLE_FAN_HPP

#include "Navigation/Flat/FlatGeoPoint.hpp"
#include "Navigation/Flat/FlatBoundingBox.hpp"

#include <vector>

class FlatTriangleFan {
public:
  typedef std::vector<FlatGeoPoint> VertexVector;

protected:
  VertexVector vs;
  FlatBoundingBox bb_self;
  short height;

public:
  FlatTriangleFan()
    :bb_self(FlatGeoPoint(0, 0)), height(0) {}

  friend class PrintHelper;

  void calc_bb();

  void add_point(const FlatGeoPoint &p);

  gcc_pure
  bool is_inside(const FlatGeoPoint &p) const;

  void clear() {
    vs.clear();
  }

  gcc_pure
  bool empty() const {
    return vs.empty();
  }
  short get_height() const {
    return height;
  }
};

#endif
