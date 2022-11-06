/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "Geo/Flat/FlatGeoPoint.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"

#include <vector>
#include <span>

class FlatTriangleFanVisitor;

class FlatTriangleFan {
  using VertexVector = std::vector<FlatGeoPoint>;

  VertexVector vs;
  FlatBoundingBox bounding_box;
  int height;

public:
  friend class PrintHelper;

  const FlatBoundingBox &CalcBoundingBox() noexcept;

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

  std::span<const FlatGeoPoint> GetVertices() const noexcept {
    return vs;
  }

  [[gnu::pure]]
  bool IsEmpty() const noexcept {
    return vs.empty();
  }

  [[gnu::pure]]
  bool IsOnlyOrigin() const noexcept {
    return vs.size() == 1;
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
  std::span<const FlatGeoPoint> GetHull(bool closed) const noexcept {
    std::span<const FlatGeoPoint> hull{vs};
    if (closed)
      /* omit the origin, because it's not part of the hull in a
         closed shape */
      hull = hull.subspan(1);
    return hull;
  }

  int GetHeight() const noexcept {
    return height;
  }

  void SetHeight(int _height) noexcept {
    height = _height;
  }

  void AcceptInRange(const FlatBoundingBox &bb,
                     FlatTriangleFanVisitor &visitor,
                     bool closed) const noexcept;
};
