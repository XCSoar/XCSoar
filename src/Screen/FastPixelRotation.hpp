/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_MATH_FAST_PIXEL_ROTATION_HPP
#define XCSOAR_MATH_FAST_PIXEL_ROTATION_HPP

#include "Math/FastRotation.hpp"

/**
 * Same as #FastIntegerRotation, but works with PixelScalar /
 * RasterPoint coordinates.
 */
class FastPixelRotation {
  FastIntegerRotation rotation;

public:
  FastPixelRotation() = default;
  FastPixelRotation(Angle angle):rotation(angle) {}

  Angle GetAngle() const {
    return rotation.GetAngle();
  }

  void SetAngle(Angle angle) {
    rotation.SetAngle(angle);
  }

  const FastPixelRotation &operator =(Angle angle) {
    SetAngle(angle);
    return *this;
  }

  gcc_pure
  RasterPoint Rotate(PixelScalar x, PixelScalar y) const {
    auto result = rotation.Rotate(x, y);
    return RasterPoint{ PixelScalar(result.first), PixelScalar(result.second) };
  }

  gcc_pure
  RasterPoint Rotate(const RasterPoint pt) const {
    return Rotate(pt.x, pt.y);
  }
};

#endif
