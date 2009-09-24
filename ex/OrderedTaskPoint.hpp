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

#ifndef ORDEREDTASKPOINT_HPP
#define ORDEREDTASKPOINT_HPP

#include "Scoring/ObservationZone.hpp"
#include "TaskPoint.hpp"
#include "SearchPoint.hpp"
#include <stdlib.h>
#include "Util.h"
#include <vector>

class TaskLeg;

class OrderedTaskPoint : public TaskPoint, public ObservationZone {
public:
  OrderedTaskPoint(const WAYPOINT & wp, bool b_scored) : 
      TaskPoint(wp), 
      boundary_scored(b_scored),
      leg_in(NULL),
      leg_out(NULL),
      active_state(NOTFOUND_ACTIVE)
    {
      clear_boundary_points();
      clear_sample_points();
    };

  enum ActiveState_t {
    NOTFOUND_ACTIVE = 0,
    BEFORE_ACTIVE,
    CURRENT_ACTIVE,
    AFTER_ACTIVE
  };

  // call this to update the object when its neighbours
  // have changed
  virtual void update_geometry() = 0;

  virtual void set_leg_out(TaskLeg*);

  virtual void set_leg_in(TaskLeg*);

  TaskLeg* get_leg_out();
  
  TaskLeg* get_leg_in();
  
  ActiveState_t getActiveState() const {
    return active_state;
  }

  void set_distance_remaining(double val) {
    distance_remaining = val;
  }
  void set_distance_nominal(double val) {
    distance_nominal = val;
  }
  void set_distance_scored(double val) {
    distance_nominal = val;
  }
  void set_distance_travelled(double val) {
    distance_nominal = val;
  }    

protected:
  bool boundary_scored;
  bool scan_active(OrderedTaskPoint* atp);
  double scan_distance_nominal();
  double scan_distance_remaining(const GEOPOINT &ref);
  double scan_distance_scored(const GEOPOINT &ref);
  double scan_distance_travelled(const GEOPOINT &ref);
public:

  virtual GEOPOINT get_reference_nominal_origin();

  virtual GEOPOINT get_reference_nominal_destination();

  virtual GEOPOINT get_reference_scored_origin();

  virtual GEOPOINT get_reference_scored_destination();

  virtual GEOPOINT get_reference_travelled_origin();

  virtual GEOPOINT get_reference_travelled_destination();

  virtual GEOPOINT get_reference_remaining_origin();

  virtual GEOPOINT get_reference_remaining_destination();

  const std::vector<SEARCH_POINT>& get_search_points();
  const std::vector<SEARCH_POINT>& get_boundary_points() const;

  virtual void default_boundary_points();
  virtual void prune_boundary_points();
  virtual void prune_sample_points();

  void set_search_max(const SEARCH_POINT &i) {
    search_max = i;
  }

  void set_search_min(const SEARCH_POINT &i) {
    search_min = i;
  }

  const SEARCH_POINT& get_search_max() {
    return search_max;
  }

  const SEARCH_POINT& get_search_min() {
    return search_min;
  }

  virtual void clear_boundary_points() {
    boundary_points.clear();
    search_max.Location = getLocation();
    search_min.Location = getLocation();
  }  
  virtual void clear_sample_points() {
    sampled_points.clear();
  }  

  virtual bool update_sample(const GEOPOINT&);

;

protected:

  TaskLeg* leg_out;
  TaskLeg* leg_in;
  ActiveState_t active_state;

  double distance_nominal;   
  double distance_scored;    
  double distance_remaining; 
  double distance_travelled; 
private:
  std::vector<SEARCH_POINT> sampled_points;
  std::vector<SEARCH_POINT> boundary_points;
  SEARCH_POINT search_max;
  SEARCH_POINT search_min;
};


#endif
