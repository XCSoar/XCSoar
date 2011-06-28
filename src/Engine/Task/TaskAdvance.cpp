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
#include "TaskAdvance.hpp"
#include "Tasks/BaseTask/TaskPoint.hpp"
#include "TaskPoints/StartPoint.hpp"
#include "TaskPoints/AATPoint.hpp"
#include "Tasks/BaseTask/IntermediatePoint.hpp"
#include "Navigation/Aircraft.hpp"

TaskAdvance::TaskAdvance():
    m_armed(false),
    m_request_armed(false) {}

void
TaskAdvance::reset()
{
  m_armed = false;
  m_request_armed = false;
}

bool 
TaskAdvance::state_ready(const TaskPoint &tp,
                         const AIRCRAFT_STATE &state,
                         const bool x_enter, 
                         const bool x_exit) const
{
  if (tp.GetType() == TaskPoint::START)
    return x_exit;

  if (tp.GetType() == TaskPoint::AAT) {
    const AATPoint *ap = (const AATPoint *)&tp;
    return aat_state_ready(ap->has_entered(), ap->close_to_target(state));
  } else if (tp.is_intermediate()) {
    const IntermediateTaskPoint *ip = (const IntermediateTaskPoint *)&tp;
    return ip->has_entered();
  }
  return false;
}

bool 
TaskAdvance::aat_state_ready(const bool has_entered,
                             const bool close_to_target) const
{
  return has_entered;
}

void 
TaskAdvance::set_armed(const bool do_armed) 
{
  m_armed = do_armed;
  m_request_armed = false;
  update_state();
}

bool 
TaskAdvance::toggle_armed() 
{
  m_armed = !m_armed;
  if (m_armed)
    m_request_armed = false;

  update_state();
  return m_armed;
}
