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

#ifndef AATPOINT_HPP
#define AATPOINT_HPP

#include "OrderedTaskPoint.hpp"
#include "IntermediatePoint.h"

class AATPoint : public IntermediatePoint {
public:
  AATPoint(const TaskProjection& tp,
           const WAYPOINT & wp) : 
    IntermediatePoint(tp,wp,true), 
    TargetLocked(false), 
    TargetLocation(wp.Location)
    {
    }  

  virtual GEOPOINT get_reference_remaining() const;
  
  virtual GEOPOINT get_reference_travelled() const;
  
  virtual GEOPOINT get_reference_scored() const;

  virtual double getElevation() const;

  virtual bool update_sample(const AIRCRAFT_STATE&);

  virtual void print(std::ostream& f, const int item=0) const;

  virtual void set_range(const double p);

  virtual void update_projection();

protected:
  GEOPOINT TargetLocation;
  bool TargetLocked;
  bool check_target(const AIRCRAFT_STATE&);
  bool check_target_inside(const AIRCRAFT_STATE&);
  bool check_target_outside(const AIRCRAFT_STATE&);
  void update_isoline();
};

#endif
