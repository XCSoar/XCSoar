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
#ifndef TASK_BEHAVIOUR_HPP
#define TASK_BEHAVIOUR_HPP

#include "Tasks/PathSolvers/Contests.hpp"
#include "OrderedTaskBehaviour.hpp"

struct AIRCRAFT_STATE;

/**
 *  Class defining options for task system.
 *  Typical uses might be default values, and simple aspects of task behaviour.
 */
class TaskBehaviour 
{
public:

/** 
 * Constructor, sets default task behaviour
 * 
 */
  TaskBehaviour();

  bool optimise_targets_range; /**< Option to enable positionining of AAT targets to achieve desired AAT minimum task time */
  bool optimise_targets_bearing; /**< Option to enable positioning of AAT targets at optimal point on isoline */
  bool auto_mc; /**< Option to enable calculation and setting of auto MacCready */

  /**
   * Enumeration of auto MC modes
   */
  enum AutoMCMode_t {
    AUTOMC_FINALGLIDE = 0, /**< Final glide only */
    AUTOMC_CLIMBAVERAGE, /**< Climb average */
    AUTOMC_BOTH /**< Final glide if above FG, else climb average */
  };

  AutoMCMode_t auto_mc_mode; /**< Options for auto MC calculations */
  
  bool calc_cruise_efficiency; /**< Option to enable calculation of cruise efficiency */
  bool calc_effective_mc; /**< Option to enable calculation of effective mc */
  bool calc_glide_required; /**< Option to enable calculation of required sink rate for final glide */
  bool goto_nonlandable; /**< Option to enable Goto tasks for non-landable waypoints */

  fixed risk_gamma; /**< Compensation factor for risk at low altitude */

  bool enable_olc; /**< Whether to do online OLC optimisation */

  Contests contest; /**< Rule set to scan for in OLC */
  unsigned contest_handicap; /**< Handicap factor */

  fixed safety_mc; /**< Safety MacCready value (m/s) used by abort task */
  bool safety_mc_use_current; /**< Whether to use safety mc value or current task polar MC */

  fixed safety_height_terrain; /**< Minimum height above terrain for arrival height at non-landable waypoint (m) */
  fixed safety_height_arrival; /**< Minimum height above terrain for arrival height at landable waypoint (m) */

  fixed start_max_speed_margin; /**< Margin in maximum ground speed (m/s) allowed in start sector */
  unsigned start_max_height_margin; /**< Margin in maximum height (m) allowed in start sector */

  OrderedTaskBehaviour ordered_defaults; /**< Defaults for ordered task */

/** 
 * Convenience function (used primarily for testing) to disable
 * all expensive task behaviour functions.
 */
  void all_off();

/** 
 * Return safety MC value (based on options)
 * 
 * @param fallback_mc Current glide polar mc value (m/s) 
 *
 * @return Safety MC value (m/s)
 */
  fixed get_safety_mc(const fixed fallback_mc) const;
};

#endif
