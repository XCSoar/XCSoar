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

#include "TaskLeg.h"
#include "ScoredTaskPoint.hpp"

class OrderedTaskPoint : 
  public TaskLeg,
  public ScoredTaskPoint
{
public:
  OrderedTaskPoint(const TaskProjection& tp,
                   const WAYPOINT & wp, 
                   const bool b_scored) : 
    ScoredTaskPoint(tp, wp, b_scored),
    tp_previous(NULL),
    tp_next(NULL),
    active_state(NOTFOUND_ACTIVE),
    TaskLeg(*this)
    {
    };

  virtual ~OrderedTaskPoint() {};

  enum ActiveState_t {
    NOTFOUND_ACTIVE = 0,
    BEFORE_ACTIVE,
    CURRENT_ACTIVE,
    AFTER_ACTIVE
  };

  // call this to update the object when its neighbours
  // have changed
  virtual void update_geometry() = 0;

  virtual void set_neighbours(OrderedTaskPoint* prev,
                              OrderedTaskPoint* next);

  OrderedTaskPoint* get_previous() const;
  OrderedTaskPoint* get_next() const;
  
  ActiveState_t getActiveState() const {
    return active_state;
  }

  bool scan_active(OrderedTaskPoint* atp);

  virtual void print(std::ostream& f, const AIRCRAFT_STATE& state,
                     const int item=0) const;

  const std::vector<SearchPoint>& get_search_points();

  virtual const GeoVector get_vector_remaining(const AIRCRAFT_STATE &) const {
    return vector_remaining;
  }
  virtual const GeoVector get_vector_travelled() const {
    return vector_travelled;
  }
  virtual const GeoVector get_vector_planned() const {
    return vector_planned;
  }

  GLIDE_RESULT glide_solution_travelled(const AIRCRAFT_STATE &, 
                                        const GlidePolar &polar,
                                        const double minH=0) const;
  GLIDE_RESULT glide_solution_planned(const AIRCRAFT_STATE &, 
                                      const GlidePolar &polar,
                                      const double minH=0) const;

private:

  /**
   * @supplierCardinality 0..1 
   */
  OrderedTaskPoint* tp_next;

  /**
   * @supplierCardinality 0..1 
   */
  OrderedTaskPoint* tp_previous;

protected:
  ActiveState_t active_state;

  double double_leg_distance(const GEOPOINT &ref) const;

};


#endif
