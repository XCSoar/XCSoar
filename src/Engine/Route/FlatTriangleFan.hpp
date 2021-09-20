/* Copyright_License {

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

#ifndef FLAT_TRIANGLE_FAN_HPP
#define FLAT_TRIANGLE_FAN_HPP

#include "Geo/Flat/FlatGeoPoint.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"
#include "util/ConstBuffer.hxx"

#include <vector>

class FlatTriangleFan {
  typedef std::vector<FlatGeoPoint> VertexVector;

protected:
  VertexVector vs;
  FlatBoundingBox bounding_box;
  int height;

public:
  friend class PrintHelper;

  void CalcBoundingBox() noexcept;

  /**
   * Add the origin to an empty
   */
  void AddOrigin(const AFlatGeoPoint &origin, size_t reserve) noexcept;

  void AddPoint(FlatGeoPoint p) noexcept;

  /**
   * Finish the point list.
   *
   * @param closed true if this is a closed circle and the origin is
   * not part of the hull
   * @return true if the fan is valid
   */
  bool CommitPoints(bool closed) noexcept;

  /**
   * @param closed true if this is a closed shape and the origin is
   * not part of the hull
   */
  [[gnu::pure]]
  bool IsInside(FlatGeoPoint p, bool closed) const noexcept;

  void Clear() noexcept {
    vs.clear();
  }

  [[gnu::pure]]
  bool IsEmpty() const noexcept {
    return vs.empty();
  }

  AFlatGeoPoint GetOrigin() const noexcept {
    return AFlatGeoPoint(vs.front(), height);
  }

  /**
   * Returns a list of points describing the hull.
   *
   * @param closed true if this is a closed circle and the origin is
   * not part of the hull
   */
  [[gnu::pure]]
  ConstBuffer<FlatGeoPoint> GetHull(bool closed) const noexcept {
    ConstBuffer<FlatGeoPoint> hull(&vs.front(), vs.size());
    if (closed)
      /* omit the origin, because it's not part of the hull in a
         closed shape */
      hull.pop_front();
    return hull;
  }

  int GetHeight() const noexcept {
    return height;
  }
};

#endif
