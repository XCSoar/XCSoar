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

#ifndef XCSOAR_COMPARE_PROJECTION_HPP
#define XCSOAR_COMPARE_PROJECTION_HPP

#include "Geo/Quadrilateral.hpp"

class WindowProjection;

/**
 * This class remembers the screen bounds of an existing Projection
 * object, and compares it after a change.  It is used to check if
 * calculation results from the previous frame are still valid, or if
 * they should be discarded.
 */
class CompareProjection {
  struct FourCorners : GeoQuadrilateral {
    FourCorners() = default;
    FourCorners(const WindowProjection &projection) noexcept;
  };

  FourCorners corners;

  double latitude_cos;

  double max_delta = -1;

public:
  /**
   * Creates a "cleared" object, so that comparisons are always false.
   */
  CompareProjection() noexcept = default;

  explicit CompareProjection(const WindowProjection &projection) noexcept;

  /**
   * Clears the object, so that comparisons are always false.  Useful
   * to Invalidate a cache.
   */
  void Clear() noexcept {
    max_delta = -1;
  }

  bool IsDefined() const noexcept {
    return max_delta > 0;
  }

  bool Compare(const CompareProjection &other) const noexcept;

  /**
   * Is the new projection close enough to the saved one?
   */
  bool Compare(const WindowProjection &projection) const noexcept {
    return Compare(CompareProjection(projection));
  }

  bool CompareAndUpdate(const CompareProjection &other) noexcept;

  /**
   * Is the new projection close enough to the saved one?  If not,
   * then the saved one is updated.
   */
  bool CompareAndUpdate(const WindowProjection &projection) noexcept {
    return CompareAndUpdate(CompareProjection(projection));
  }
};

#endif
