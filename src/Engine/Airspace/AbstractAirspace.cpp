/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "AbstractAirspace.hpp"
#include <assert.h>


bool 
AbstractAirspace::inside(const AIRCRAFT_STATE& state) const
{
  return ((state.NavAltitude >= m_base.Altitude) &&
          (state.NavAltitude <= m_top.Altitude) &&
          inside(state.Location));
}


void 
AbstractAirspace::set_ground_level(const fixed alt) 
{
  m_base.set_ground_level(alt);
  m_top.set_ground_level(alt);
}


void 
AbstractAirspace::set_flight_level(const AtmosphericPressure &press) 
{
  m_base.set_flight_level(press);
  m_top.set_flight_level(press);
}


AirspaceInterceptSolution 
AbstractAirspace::intercept_vertical(const AIRCRAFT_STATE &state,
                                     const AirspaceAircraftPerformance& perf,
                                     const fixed& distance) const
{
  AirspaceInterceptSolution solution;
  solution.distance = distance;
  solution.elapsed_time = perf.solution_vertical(solution.distance, 
                                                 state.NavAltitude,
                                                 m_base.Altitude, 
                                                 m_top.Altitude,
                                                 solution.altitude);
  return solution;
}


AirspaceInterceptSolution 
AbstractAirspace::intercept_horizontal(const AIRCRAFT_STATE &state,
                                       const AirspaceAircraftPerformance& perf,
                                       const fixed& distance_start,
                                       const fixed& distance_end,
                                       const bool lower) const
{
  AirspaceInterceptSolution solution;

  if (lower && m_base.is_terrain()) {
    // impossible to be lower than terrain
    return solution;
  }

  solution.altitude = lower? m_base.Altitude: m_top.Altitude;
  solution.elapsed_time = perf.solution_horizontal(distance_start, 
                                                   distance_end,
                                                   state.NavAltitude,
                                                   solution.altitude,
                                                   solution.distance);
  return solution;
}


bool 
AbstractAirspace::intercept(const AIRCRAFT_STATE &state,
                            const AirspaceAircraftPerformance& perf,
                            AirspaceInterceptSolution &solution,
                            const GEOPOINT& loc_start,
                            const GEOPOINT& loc_end) const
{
  const fixed distance_start = state.Location.distance(loc_start);
  const fixed distance_end = (loc_start==loc_end)? 
    distance_start:state.Location.distance(loc_end);

  AirspaceInterceptSolution solution_this;

  // need to scan three sides, top, far, bottom (if not terrain)

  AirspaceInterceptSolution solution_candidate;
  solution_candidate = intercept_vertical(state, perf, distance_end);
  if (solution_candidate.valid() && 
      ((solution_candidate.elapsed_time < solution_this.elapsed_time) ||
       negative(solution_this.elapsed_time))) {
    
    solution_this = solution_candidate;
  }

  solution_candidate = intercept_horizontal(state, perf, distance_start, distance_end, false);
  if (solution_candidate.valid() && 
      ((solution_candidate.elapsed_time < solution_this.elapsed_time) ||
       negative(solution_this.elapsed_time))) {
    solution_this = solution_candidate;
  }
  
  if (!m_base.is_terrain()) {
    solution_candidate = intercept_horizontal(state, perf, distance_start, distance_end, true);
    if (solution_candidate.valid() && 
        ((solution_candidate.elapsed_time < solution_this.elapsed_time) ||
         negative(solution_this.elapsed_time))) {
      solution_this = solution_candidate;
    }
  }

  if (solution_this.valid()) {
    solution = solution_this;
    if (solution.distance == distance_start) {
      solution.location = loc_start;
    } else if (solution.distance == distance_end) {
      solution.location = loc_end;
    } else if (distance_end>distance_start) {
      const fixed t = solution.distance / (distance_end-distance_start);
      solution.location = loc_start+(loc_end-loc_start)*t;
    } else {
      solution.location = loc_start;
    }
    assert(solution.distance>=0);
    return true;
  } else {
    return false;
  }
}


bool 
AbstractAirspace::intercept(const AIRCRAFT_STATE &state,
                            const GeoVector& vec,
                            const AirspaceAircraftPerformance& perf,
                            AirspaceInterceptSolution &solution) const
{
  AirspaceIntersectionVector vis = intersects(state.Location, vec);
  if (vis.empty()) {
    return false;
  }

  AirspaceInterceptSolution this_solution;
  for (AirspaceIntersectionVector::const_iterator it = vis.begin();
       it != vis.end(); ++it) {
    intercept(state, perf, this_solution, it->first, it->second);
  }

  if (this_solution.valid()) {
    solution = this_solution;
    return true;
  } else {
    return false;
  }
}

const tstring 
AbstractAirspace::get_type_text(const bool concise) const
{
  return airspace_class_as_text(Type, concise);
}


const tstring 
AbstractAirspace::get_name_text(const bool concise) const
{
  if (concise) {
    return Name;
  } else {
    return Name + _T(" ") + airspace_class_as_text(Type);
  }
}


const tstring 
AbstractAirspace::get_base_text(const bool concise) const
{
  return m_base.get_as_text(concise);
}

const tstring 
AbstractAirspace::get_top_text(const bool concise) const
{
  return m_top.get_as_text(concise);
}
