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
#ifndef ORDEREDTASK_BEHAVIOUR_HPP
#define ORDEREDTASK_BEHAVIOUR_HPP

#include "Math/fixed.hpp"
#include "Util/Serialisable.hpp"

class AIRCRAFT_STATE;
class TaskBehaviour;

class OrderedTaskBehaviour:
  public Serialisable
{
public:
  friend class Serialiser;
  OrderedTaskBehaviour();

  bool task_scored; /**< Option to enable calculation of scores, and protect against task changes if flight/task has started */
  fixed aat_min_time; /**< Desired AAT minimum task time (s) */
  fixed start_max_speed; /**< Maximum ground speed (m/s) allowed in start sector */
  unsigned start_max_height; /**< Maximum height (m) allowed in start sector */
  unsigned start_max_height_ref; /**< Reference for max start height (0=AGL, 1=abs) */
  unsigned finish_min_height; /**< Minimum height AGL (m) allowed to finish */
  bool fai_finish; /**< Whether ordered task finish requires FAI height rule */
  unsigned min_points; /**< Minimum number of turnpoints including start/finish */
  unsigned max_points; /**< Maximum number of turnpoints including start/finish */
  bool homogeneous_tps; /**< Whether all turnpoints except start/finish are same type */
  bool is_closed; /**< Whether start/finish turnpoints must be the same */
  bool start_requires_arm; /**< Whether start points needs to be armed */

/** 
 * Check whether aircraft speed is within start speed limits
 * 
 * @param state Aircraft state
 * @param with_margin Whether to use margin for minor rule violation
 * 
 * @return True if within limits
 */
  bool check_start_speed(const AIRCRAFT_STATE &state, 
                         const TaskBehaviour& behaviour,
                         const bool with_margin=false) const;

/** 
 * Check whether aircraft height is within start height limit
 * 
 * @param state Aircraft state
 * @param with_margin Whether to use margin for minor rule violation
 * 
 * @return True if within limits
 */
  bool check_start_height(const AIRCRAFT_STATE &state,
                          const TaskBehaviour& behaviour,
                          const bool with_margin=false) const;

/** 
 * Check whether aircraft height is within finish height limit
 * 
 * @param state Aircraft state
 * 
 * @return True if within limits
 */
  bool check_finish_height(const AIRCRAFT_STATE &state) const;


/** 
 * Convenience function (used primarily for testing) to disable
 * all expensive task behaviour functions.
 */
  void all_off();

  bool is_fixed_size() const;

public:
};

#endif
