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
#include "TaskAdvanceSmart.hpp"
#include "Tasks/BaseTask/TaskPoint.hpp"
#include "TaskPoints/StartPoint.hpp"
#include "TaskPoints/AATPoint.hpp"
#include "Tasks/BaseTask/IntermediatePoint.hpp"
#include "Navigation/Aircraft.hpp"
#include "OrderedTaskBehaviour.hpp"

TaskAdvanceSmart::TaskAdvanceSmart(const OrderedTaskBehaviour& behaviour):
  TaskAdvance(),
  m_state(TaskAdvance::MANUAL),
  m_task_behaviour(behaviour)
{
}

bool 
TaskAdvanceSmart::ready_to_advance(const TaskPoint &tp,
                                   const AIRCRAFT_STATE &state,
                                   const bool x_enter, 
                                   const bool x_exit)
{
  const bool m_state_ready = state_ready(tp, state, x_enter, x_exit);

  if (m_armed) {
    m_request_armed = false;
  }

  if (const StartPoint* sp = dynamic_cast<const StartPoint*>(&tp)) {
    if (m_task_behaviour.start_requires_arm) {
      if (m_armed) {
        m_state = TaskAdvance::START_ARMED;
      } else {
        m_state = TaskAdvance::START_DISARMED;
        if (sp->isInSector(state)) {
          m_request_armed = true;
        }
      }
      return m_armed && m_state_ready;
    } else {
      m_state = TaskAdvance::AUTO;
      return m_state_ready;
    }
  } else if (dynamic_cast<const AATPoint*>(&tp)) {
    if (m_armed) {
      m_state = TaskAdvance::TURN_ARMED;
    } else {
      m_state = TaskAdvance::TURN_DISARMED;
      if (m_state_ready) {
        m_request_armed = true;
      }
    }
    return m_armed && m_state_ready;
  } else if (dynamic_cast<const IntermediatePoint*>(&tp)) {
    m_state = TaskAdvance::AUTO;
    return m_state_ready;
  } 
  return false;
}


TaskAdvance::TaskAdvanceState_t 
TaskAdvanceSmart::get_advance_state() const
{
  return m_state;
}
