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

#include "Math/fixed.hpp"

class AIRCRAFT_STATE;

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
  bool calc_cruise_efficiency; /**< Option to enable calculation of cruise efficiency */

  bool calc_glide_required; /**< Option to enable calculation of required sink rate for final glide */

  bool goto_nonlandable; /**< Option to enable Goto tasks for non-landable waypoints */

  bool task_scored; /**< Option to enable calculation of scores, and protect against task changes
                       if flight/task has started */

  fixed aat_min_time; /**< Desired AAT minimum task time (s) */
  fixed safety_height_terrain; /**< Minimum height above terrain for arrival height at non-landable waypoint (m) */
  fixed safety_height_arrival; /**< Minimum height above terrain for arrival height at landable waypoint (m) */
  fixed start_max_speed; /**< Maximum ground speed (m/s) allowed in start sector */


/** 
 * Convenience function (used primarily for testing) to disable
 * all expensive task behaviour functions.
 */
  void all_off();

/** 
 * Check whether aircraft speed is within start speed limits
 * 
 * @param state Aircraft state
 * 
 * @return True if within limits
 */
  bool check_start_speed(const AIRCRAFT_STATE &state) const;
};

#endif
