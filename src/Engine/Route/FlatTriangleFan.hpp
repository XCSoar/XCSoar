/* Copyright_License {

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

#ifndef FLAT_TRIANGLE_FAN_HPP
#define FLAT_TRIANGLE_FAN_HPP

#include "Geo/Flat/FlatGeoPoint.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"
#include "Util/ConstBuffer.hxx"

#include <vector>

class FlatTriangleFan {
  typedef std::vector<FlatGeoPoint> VertexVector;

protected:
  VertexVector vs;
  FlatBoundingBox bounding_box;
  int height;

public:
  friend class PrintHelper;

  void CalcBoundingBox();

  /**
   * Add the origin to an empty
   */
  void AddOrigin(const AFlatGeoPoint &origin, size_t reserve);

  void AddPoint(FlatGeoPoint p);

  /**
   * Finish the point list.
   *
   * @param closed true if this is a closed circle and the origin is
   * not part of the hull
   * @return true if the fan is valid
   */
  bool CommitPoints(bool closed);

  /**
   * @param closed true if this is a closed shape and the origin is
   * not part of the hull
   */
  gcc_pure
  bool IsInside(FlatGeoPoint p, bool closed) const;

  void Clear() {
    vs.clear();
  }

  gcc_pure
  bool IsEmpty() const {
    return vs.empty();
  }

  AFlatGeoPoint GetOrigin() const {
    return AFlatGeoPoint(vs.front(), height);
  }

  /**
   * Returns a list of points describing the hull.
   *
   * @param closed true if this is a closed circle and the origin is
   * not part of the hull
   */
  gcc_pure
  ConstBuffer<FlatGeoPoint> GetHull(bool closed) const {
    ConstBuffer<FlatGeoPoint> hull(&vs.front(), vs.size());
    if (closed)
      /* omit the origin, because it's not part of the hull in a
         closed shape */
      hull.pop_front();
    return hull;
  }

  int GetHeight() const {
    return height;
  }
};

#endif
