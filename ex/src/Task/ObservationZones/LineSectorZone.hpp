/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

  M Roberts (original release)
  Robin Birch <robinb@ruffnready.co.uk>
  Samuel Gisiger <samuel.gisiger@triadis.ch>
  Jeff Goodenough <jeff@enborne.f2s.com>
  Alastair Harrison <aharrison@magic.force9.co.uk>
  Scott Penrose <scottp@dd.com.au>
  John Wharington <jwharington@gmail.com>
  Lars H <lars_hn@hotmail.com>
  Rob Dunning <rob@raspberryridgesheepfarm.com>
  Russell King <rmk@arm.linux.org.uk>
  Paolo Ventafridda <coolwind@email.it>
  Tobias Lohner <tobias@lohner-net.de>
  Mirek Jezek <mjezek@ipplc.cz>
  Max Kellermann <max@duempel.org>

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

#ifndef LINESECTORZONE_HPP
#define LINESECTORZONE_HPP
#include "SymmetricSectorZone.hpp"

class LineSectorZone: 
  public SymmetricSectorZone 
{
public:
  LineSectorZone(const GEOPOINT loc, const double radius=1000.0):
    SymmetricSectorZone(loc,radius,180.0)
  {};

/** 
 * Clone with shift (for use when we want to create a new taskpoint
 * retaining the OZ type of another) 
 * 
 * @param _location Location of copy
 * 
 * @return New object
 */
  virtual LineSectorZone* clone(const GEOPOINT &_location) {
    return new LineSectorZone(_location,getRadius());
  }

/** 
 * Check transition constraints -- for lines, both points have to
 * be within radius of OZ (otherwise it is a circumference border crossing)
 * 
 * @param ref_now Current aircraft state
 * @param ref_last Previous aircraft state
 * 
 * @return True if constraints are satisfied
 */
  virtual bool transition_constraint(const AIRCRAFT_STATE & ref_now, 
                                     const AIRCRAFT_STATE & ref_last) {
    return CylinderZone::isInSector(ref_now) && CylinderZone::isInSector(ref_last);
  }

  GEOPOINT get_boundary_parametric(double) ;  

  virtual double score_adjustment();

/** 
 * Set length property
 * 
 * @param new_length Length (m) of line
 */
  virtual void setLength(double new_length) {
    setRadius(new_length/2.0);
  }
  
/** 
 * Get length property value
 * 
 * @return Length (m) of line
 */
  double getLength() const {
    return getRadius()*2.0;
  }

/** 
 * Test whether an OZ is equivalent to this one
 * 
 * @param other OZ to compare to
 * 
 * @return True if same type and OZ parameters
 */

  virtual bool equals(const ObservationZonePoint* other) const;

};

#endif
