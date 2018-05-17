/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

struct RoutePlannerConfig
{
  enum class Mode {
    NONE,
    TERRAIN,
    AIRSPACE,
    BOTH,
  };

  enum class Polar {
    TASK,
    SAFETY,
  };

  enum class ReachMode {
    OFF,
    STRAIGHT,
    TURNING,
  };

  Mode mode;
  bool allow_climb;
  bool use_ceiling;

  /** Minimum height above terrain for arrival height at non-landable waypoint,
      and for terrain clearance en-route (m) */
  double safety_height_terrain;

  /** Whether to allow turns around obstacles in reach calculations, or just
      straight line */
  ReachMode reach_calc_mode;

  /** Whether reach/abort calculations will use the task or safety polar */
  Polar reach_polar_mode;

  void SetDefaults();

  bool IsTerrainEnabled() const {
    return mode == Mode::TERRAIN || mode == Mode::BOTH;
  }

  bool IsAirspaceEnabled() const {
    return mode == Mode::AIRSPACE || mode== Mode::BOTH;
  }

  bool IsReachEnabled() const {
    return reach_calc_mode != ReachMode::OFF;
  }

  bool IsTurningReachEnabled() const {
    return reach_calc_mode == ReachMode::TURNING;
  }
};

#endif
