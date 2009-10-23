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


#ifndef STARTPOINT_HPP
#define STARTPOINT_HPP

#include "OrderedTaskPoint.hpp"

class StartPoint : public OrderedTaskPoint {
public:
  StartPoint(const TaskProjection& tp,
             const WAYPOINT & wp) : 
    OrderedTaskPoint(tp,wp,false), 
    enabled(true) 
    {

    };

  virtual void set_leg_in(TaskLeg* the_leg);

  // allow access to forward scan
  bool scan_active(OrderedTaskPoint* atp) {
    return OrderedTaskPoint::scan_active(atp);
  }

  double scan_distance_max() {
    return OrderedTaskPoint::scan_distance_max();
  };
  double scan_distance_min() {
    return OrderedTaskPoint::scan_distance_min();
  };
  double scan_distance_nominal() {
    return OrderedTaskPoint::scan_distance_nominal();
  };
  double scan_distance_planned() {
    return OrderedTaskPoint::scan_distance_planned();
  };
  double scan_distance_remaining(const GEOPOINT &ref) {
    return OrderedTaskPoint::scan_distance_remaining(ref);
  };
  double scan_distance_travelled(const GEOPOINT &ref) {
    return OrderedTaskPoint::scan_distance_travelled(ref);
  };
  double scan_distance_scored(const GEOPOINT &ref) {
    return OrderedTaskPoint::scan_distance_scored(ref);
  }  

  virtual bool transition_exit(const AIRCRAFT_STATE & ref_now, 
                               const AIRCRAFT_STATE & ref_last);

  virtual double getElevation();

protected:
    bool enabled;
};

#endif
