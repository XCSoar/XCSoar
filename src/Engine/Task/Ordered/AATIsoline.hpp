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

#ifndef AATISOLINE_HPP
#define AATISOLINE_HPP

#include "Geo/GeoEllipse.hpp"

class AATPoint;

/**
 * Object representing an isoline, being the locus of potential target
 * points within an AATPoint's observation zone such that all points
 * have constant double-leg distance (distance from previous max to
 * this point to next planned).
 *
 * Internally, this is represented by an ellipse in flat-earth
 * projection.
 */
class AATIsoline
{
protected:
  /** ellipse representing the isoline segment */
  const GeoEllipse ell;

public:
  /**
   * Constructor.
   *
   * @param ap The AAT point for which to calculate the Isoline
   */
  AATIsoline(const AATPoint &ap, const FlatProjection &projection);
};


#endif
