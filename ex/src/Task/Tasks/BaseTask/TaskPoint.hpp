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


#ifndef TASKPOINT_HPP
#define TASKPOINT_HPP

#include "Util/Serialisable.hpp"
#include "Navigation/ReferencePoint.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Navigation/Aircraft.hpp"
#include "Navigation/GeoVector.hpp"
#include "Util/GenericVisitor.hpp"

struct GLIDE_RESULT;
class GlidePolar;

class TaskPoint : 
  public ReferencePoint, 
  public Serialisable,
  public BaseVisitable<>
{

public:
  TaskPoint(const Waypoint & wp) : ReferencePoint(wp.Location),
                                   Elevation(wp.Altitude),
                                   waypoint(wp)
    { }

  virtual ~TaskPoint() {};

  virtual TaskPoint* clone() { return new TaskPoint(waypoint); };

  // not const because may need to perform lookup and save
  virtual double getElevation() const;

  virtual GEOPOINT get_reference_remaining() const;
  
  virtual const GeoVector get_vector_remaining(const AIRCRAFT_STATE &) const;

  GLIDE_RESULT glide_solution_remaining(const AIRCRAFT_STATE &, 
                                        const GlidePolar &polar,
                                        const double minH=0) const;
  GLIDE_RESULT glide_solution_sink(const AIRCRAFT_STATE &, 
                                   const GlidePolar &polar,
                                   const double S) const;

  virtual GLIDE_RESULT glide_solution_travelled(const AIRCRAFT_STATE &, 
                                                const GlidePolar &polar,
                                                const double minH=0) const;

  virtual GLIDE_RESULT glide_solution_planned(const AIRCRAFT_STATE &, 
                                              const GlidePolar &polar,
                                              const double minH=0) const;

  virtual void set_range(const double p) {};

#ifdef DO_PRINT
  virtual void print(std::ostream& f, const AIRCRAFT_STATE &state) const;
#endif

  virtual bool has_entered() const {
    return false;
  }

  virtual AIRCRAFT_STATE get_state_entered() const {
    // this should never get called
    AIRCRAFT_STATE null_state;
    return null_state;
  }
  const Waypoint& get_waypoint() const {
    return waypoint;
  }

protected:
  const Waypoint waypoint; // local copy
  const double Elevation;
public:
  DEFINE_VISITABLE()
};

#endif
