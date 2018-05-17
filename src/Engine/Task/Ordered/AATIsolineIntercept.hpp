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

#ifndef AATISOLINEINTERCEPT_HPP
#define AATISOLINEINTERCEPT_HPP

#include "AATIsoline.hpp"

/**
 * Specialisation of AATIsoline to calculate intercepts between
 * a line extending from the aircraft to the isoline.
 */
class AATIsolineIntercept: public AATIsoline
{
public:
  /**
   * Constructor.
   *
   * @param ap The AAT point for which the isoline is sought
   *
   * @return Initialised object
   */
  AATIsolineIntercept(const AATPoint &ap);

  /**
   * Calculate intercept location.  Test line bearing is from previous
   * max/achieved point, through aircraft, adjusted by bearing_offset.
   *
   * \todo
   * - adjust for bearing_offset (currently not implemented)
   *
   * @param ap AAT point associated with the isoline
   * @param state Aircraft state from which intercept line originates
   * @param bearing_offset Offset of desired bearing between cruise track from previous and intercept line
   * @param ip Set location of intercept point (if returned true)
   *
   * @return True if intercept is found and within OZ
   */
  bool Intercept(const AATPoint &ap, const AircraftState &state,
                 const double bearing_offset, GeoPoint &ip) const;
};

#endif
