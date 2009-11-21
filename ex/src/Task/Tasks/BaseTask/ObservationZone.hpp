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


#ifndef OBSERVATIONZONE_HPP
#define OBSERVATIONZONE_HPP

struct GEOPOINT;
#include "Navigation/Aircraft.hpp"

class ObservationZone {
public:
  ObservationZone() {

    };

  /** returns true if reference point is inside sector
   */
    virtual bool isInSector(const AIRCRAFT_STATE & ref) const = 0;

/** 
 * Check transition constraints
 * 
 * @param ref_now Current aircraft state
 * @param ref_last Previous aircraft state
 * 
 * @return True if constraints are satisfied
 */
  virtual bool transition_constraint(const AIRCRAFT_STATE & ref_now, 
                                     const AIRCRAFT_STATE & ref_last) = 0;

  /** 
   * Check if aircraft has transitioned to inside sector
   * 
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if aircraft now inside (and was outside)
   */
    virtual bool transition_enter(const AIRCRAFT_STATE & ref_now, 
                                  const AIRCRAFT_STATE & ref_last) {
        return isInSector(ref_now) && !isInSector(ref_last)
          && transition_constraint(ref_now, ref_last);
    };

  /** 
   * Check if aircraft has transitioned to outside sector
   * 
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if aircraft now outside (and was inside)
   */
    virtual bool transition_exit(const AIRCRAFT_STATE & ref_now, 
                                 const AIRCRAFT_STATE & ref_last) {
        return transition_enter(ref_last, ref_now);
    }  

  virtual GEOPOINT get_boundary_parametric(double t) const =0;

  virtual double score_adjustment() const;
};

#endif
