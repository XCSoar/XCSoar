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

#ifndef TASK_BEHAVIOUR_HPP
#define TASK_BEHAVIOUR_HPP

#include "Tasks/PathSolvers/Contests.hpp"
#include "OrderedTaskBehaviour.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"

struct AIRCRAFT_STATE;

enum AbortTaskMode {
  atmSimple,
  atmTask,
  atmHome,
};

struct RoutePlannerConfig {
  RoutePlannerConfig():
    mode(rpNone), // default disable while experimental
    allow_climb(true),
    use_ceiling(false),
    safety_height_terrain(150.0),
    reach_calc_mode(rmTurning),
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

/**
 * variables set user preference defaults for new task and
 * new turpoints created by the task factories
 */
struct SectorDefaults
{
  SectorDefaults() :
    start_type(AbstractTaskFactory::START_CYLINDER), start_radius(1000),
        turnpoint_type(AbstractTaskFactory::AST_CYLINDER),
        turnpoint_radius(500),
        finish_type(AbstractTaskFactory::FINISH_CYLINDER), finish_radius(1000)
  {
  }

  /** default start type for new tasks */
  AbstractTaskFactory::LegalPointType_t start_type;
  /** default start radius or line length for new tasks */
  fixed start_radius;
  /** default intermediate type for new tasks */
  AbstractTaskFactory::LegalPointType_t turnpoint_type;
  /** default intermediate point radius for new tasks */
  fixed turnpoint_radius;
  /** default finish type for new tasks */
  AbstractTaskFactory::LegalPointType_t finish_type;
  /** default finish radius or line length for new tasks */
  fixed finish_radius;
};

/**
 *  Class defining options for task system.
 *  Typical uses might be default values, and simple aspects of task behaviour.
 */
class TaskBehaviour 
{
public:
  /**
   * Constructor, sets default task behaviour
   */
  TaskBehaviour();

  /**
   * Enumeration of factory types.  This is the set of
   * types of ordered task that can be created.
   */
  enum Factory_t {
    FACTORY_FAI_GENERAL = 0,
    FACTORY_FAI_TRIANGLE,
    FACTORY_FAI_OR,
    FACTORY_FAI_GOAL,
    FACTORY_RT,
    FACTORY_AAT,
    FACTORY_MIXED,
    FACTORY_TOURING
  };

  /**
   * Option to enable positionining of AAT targets to achieve
   * desired AAT minimum task time
   */
  bool optimise_targets_range;
  /** Option to enable positioning of AAT targets at optimal point on isoline */
  bool optimise_targets_bearing;
  /** Option to enable calculation and setting of auto MacCready */
  unsigned optimise_targets_margin;
  /** Seconds additional to min time to optimise for */
  bool auto_mc;

  /** Enumeration of auto MC modes */
  enum AutoMCMode_t {
    /** Final glide only */
    AUTOMC_FINALGLIDE = 0,
    /** Climb average */
    AUTOMC_CLIMBAVERAGE,
    /** Final glide if above FG, else climb average */
    AUTOMC_BOTH
  };

  /** Options for auto MC calculations */
  AutoMCMode_t auto_mc_mode;
  
  /** Option to enable calculation of cruise efficiency */
  bool calc_cruise_efficiency;
  /** Option to enable calculation of effective mc */
  bool calc_effective_mc;
  /** Option to enable calculation of required sink rate for final glide */
  bool calc_glide_required;
  /** Option to enable Goto tasks for non-landable waypoints */
  bool goto_nonlandable;

  /** Compensation factor for risk at low altitude */
  fixed risk_gamma;

  /** Whether to do online OLC optimisation */
  bool enable_olc;

  /** Rule set to scan for in OLC */
  Contests contest;
  /** Handicap factor */
  unsigned contest_handicap;

  /** Safety MacCready value (m/s) used by abort task */
  fixed safety_mc;

  /** Minimum height above terrain for arrival height at landable waypoint (m) */
  fixed safety_height_arrival;

  /** Margin in maximum ground speed (m/s) allowed in start sector */
  fixed start_max_speed_margin;
  /** Margin in maximum height (m) allowed in start sector */
  unsigned start_max_height_margin;

  /** Default task type to use for new tasks */
  Factory_t task_type_default;

  /** Default sector info for new ordered task */
  SectorDefaults sector_defaults;

  /** Defaults for ordered task */
  OrderedTaskBehaviour ordered_defaults;

  /** Whether to maintain a thinned trace of the flight.  This is overridden by enable_olc. */
  bool enable_trace;

  /**
   * How should the Abort/Alternate task work like:
   * atmSimple: sort only by arrival height and wp type
   * atmTask: sort also by deflection from current turnpoint
   * atmHome: sort also by deflection from home
   */
  AbortTaskMode abort_task_mode;

  /** Route and reach planning */
  RoutePlannerConfig route_planner;

  /**
   * Convenience function (used primarily for testing) to disable
   * all expensive task behaviour functions.
   */
  void all_off();

};

#endif
