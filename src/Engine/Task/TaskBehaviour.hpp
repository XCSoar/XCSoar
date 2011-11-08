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

#include "Contest/ContestSolvers/Contests.hpp"
#include "OrderedTaskBehaviour.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Route/Config.hpp"
#include "Util/TypeTraits.hpp"

struct AircraftState;

enum AbortTaskMode {
  atmSimple,
  atmTask,
  atmHome,
};

/**
 * variables set user preference defaults for new task and
 * new turpoints created by the task factories
 */
struct SectorDefaults
{
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

  void SetDefaults();
};

struct TaskStartMargins {
  /** Margin in maximum ground speed (m/s) allowed in start sector */
  fixed start_max_speed_margin;
  /** Margin in maximum height (m) allowed in start sector */
  unsigned start_max_height_margin;

  void SetDefaults();
};

/**
 *  Class defining options for task system.
 *  Typical uses might be default values, and simple aspects of task behaviour.
 */
struct TaskBehaviour : public TaskStartMargins {
  /**
   * Enumeration of factory types.  This is the set of
   * types of ordered task that can be created.
   */
  enum FactoryType {
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
  enum AutoMCMode {
    /** Final glide only */
    AUTOMC_FINALGLIDE = 0,
    /** Climb average */
    AUTOMC_CLIMBAVERAGE,
    /** Final glide if above FG, else climb average */
    AUTOMC_BOTH
  };

  /** Options for auto MC calculations */
  AutoMCMode auto_mc_mode;
  
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

  /** Default task type to use for new tasks */
  FactoryType task_type_default;

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

  void SetDefaults();

  /**
   * Convenience function (used primarily for testing) to disable
   * all expensive task behaviour functions.
   */
  void DisableAll();
};

static_assert(is_trivial<TaskBehaviour>::value, "type is not trivial");

#endif
