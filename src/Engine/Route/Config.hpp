/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_ROUTE_PLANNER_CONFIG_HPP
#define XCSOAR_ROUTE_PLANNER_CONFIG_HPP

#include "Math/fixed.hpp"

struct RoutePlannerConfig {
  RoutePlannerConfig():
    mode(rpNone), // default disable while experimental
    allow_climb(true),
    use_ceiling(false),
    safety_height_terrain(150.0),
    reach_calc_mode(rmStraight),
    reach_polar_mode(rpmSafety) {}

  enum Mode {
    rpNone=0,
    rpTerrain,
    rpAirspace,
    rpBoth
  };

  enum PolarMode {
    rpmTask=0,
    rpmSafety
  };

  enum ReachMode {
    rmOff=0,
    rmStraight,
    rmTurning
  };

  Mode mode;
  bool allow_climb;
  bool use_ceiling;

  /** Minimum height above terrain for arrival height at non-landable waypoint,
      and for terrain clearance en-route (m) */
  fixed safety_height_terrain;

  /** Whether to allow turns around obstacles in reach calculations, or just
      straight line */
  ReachMode reach_calc_mode;

  /** Whether reach/abort calculations will use the task or safety polar */
  PolarMode reach_polar_mode;

  bool terrain_enabled() const {
    return (mode== rpTerrain) || (mode== rpBoth);
  }
  bool airspace_enabled() const {
    return (mode== rpAirspace) || (mode== rpBoth);
  }
  bool reach_enabled() const {
    return reach_calc_mode != rmOff;
  }
};

#endif
