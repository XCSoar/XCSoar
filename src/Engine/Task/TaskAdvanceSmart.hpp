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
#ifndef TASKADVANCESMART_HPP
#define TASKADVANCESMART_HPP

#include "TaskAdvance.hpp"

class OrderedTaskBehaviour;

/**
 * Class used to control advancement through an OrderedTask
 */
class TaskAdvanceSmart: 
  public TaskAdvance
{
public:
/** 
 * Constructor.  Sets defaults to auto-mode
 */
  TaskAdvanceSmart(const OrderedTaskBehaviour &task_behaviour);

  TaskAdvance::TaskAdvanceState_t get_advance_state() const;

/** 
 * Determine whether all conditions are satisfied for a turnpoint
 * to auto-advance based on condition of the turnpoint, transition
 * characteristics and advance mode.
 * 
 * @param tp The task point to check for satisfaction
 * @param state current aircraft state
 * @param x_enter whether this step transitioned enter to this tp
 * @param x_exit whether this step transitioned exit to this tp
 * 
 * @return true if this tp is ready to advance
 */
  bool ready_to_advance(const TaskPoint &tp,
                        const AIRCRAFT_STATE &state,
                        const bool x_enter, 
                        const bool x_exit);

protected:

  void update_state();

  TaskAdvanceState_t m_state;       /**< active advance state */

private:
  const OrderedTaskBehaviour &m_task_behaviour;

};


#endif
