/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#ifndef ROUTE_PLANNER_GLUE_HPP
#define ROUTE_PLANNER_GLUE_HPP

#include "Route/AirspaceRoute.hpp"

class RasterTerrain;

class RoutePlannerGlue {
public:
  RoutePlannerGlue(const GlidePolar& polar,
                   const Airspaces& master);

  void set_terrain(RasterTerrain* _terrain);

  void update_polar(const GlidePolar& polar,
                    const GlidePolar& safety_polar,
                    const SpeedVector& wind) {
    m_planner.update_polar(polar, safety_polar, wind);
  }

  void synchronise(const Airspaces& master,
                   const AGeoPoint& origin,
                   const AGeoPoint& destination) {
    m_planner.synchronise(master, origin, destination);
  }
  void reset() {
    m_planner.reset();
  }
  bool solve(const AGeoPoint& origin,
             const AGeoPoint& destination,
             const RoutePlannerConfig& config,
             const short h_ceiling);

  void get_solution(Route& route) const {
    m_planner.get_solution(route);
  }

  void solve_reach(const AGeoPoint& origin, const bool do_solve);

  bool find_positive_arrival(const AGeoPoint& dest,
                             short& arrival_height_reach,
                             short& arrival_height_direct) const;

  void accept_in_range(const GeoBounds& bounds,
                       TriangleFanVisitor& visitor) const;

  bool intersection(const AGeoPoint& origin,
                    const AGeoPoint& destination,
                    GeoPoint& intx) const;

  GlidePolar get_reach_polar() const;

  short get_terrain_base() const;

private:
  RasterTerrain* terrain;
  AirspaceRoute m_planner;
};

#endif
