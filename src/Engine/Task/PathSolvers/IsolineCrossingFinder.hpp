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

#ifndef ISOLINECROSSINGFINDER_HPP
#define ISOLINECROSSINGFINDER_HPP

#include "Math/ZeroFinder.hpp"

class GeoEllipse;
class AATPoint;

/**
 *  Calculate where Isoline ellipse crosses border of observation zone
 */
class IsolineCrossingFinder final : public ZeroFinder
{
  const AATPoint &aap;
  const GeoEllipse &ell;

public:
  /**
   * Constructor.  After construction, call solve() to perform the search.
   *
   * @param _aap AATPoint for which to test OZ inclusion
   * @param _ell GeoEllipse representing the isoline
   * @param xmin Min parameter of search
   * @param xmax Max parameter of search
   *
   * @return Partially initialised object
   */
  IsolineCrossingFinder(const AATPoint& _aap, const GeoEllipse &_ell,
                        const double xmin, const double xmax);

  double f(const double t);

  /**
   * Test validity of solution
   * @param t parametric location of test point
   * @return True if valid
   */
  bool valid(const double t);

  /**
   * Search for parameter value of isoline intersecting the OZ boundary
   * within min/max parameter range.
   *
   * @return Parameter value of isoline intersection
   */
  double solve();
};

#endif
