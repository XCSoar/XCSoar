/*
Copyright_License {

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

#include "GlideComputerInterface.hpp"
#include "InputEvents.hpp"
#include "GlideComputer.hpp"

void 
GlideComputerTaskEvents::set_computer(GlideComputer& computer)
{
  m_computer = &computer;
}


void 
GlideComputerTaskEvents::transition_enter(const TaskWaypoint &tp)
{
  m_computer->OnTransitionEnter();
}

void 
GlideComputerTaskEvents::transition_alternate() 
{
  InputEvents::processGlideComputer(GCE_ALTERNATE_CHANGED);
}

void 
GlideComputerTaskEvents::request_arm(const TaskWaypoint &tp)
{
  InputEvents::processGlideComputer(GCE_ARM_READY);
}

void 
GlideComputerTaskEvents::active_advanced(const TaskWaypoint &tp, const int i)
{
  InputEvents::processGlideComputer(GCE_TASK_NEXTWAYPOINT);
}

void 
GlideComputerTaskEvents::task_start()
{
  InputEvents::processGlideComputer(GCE_TASK_START);
  m_computer->OnStartTask();
}

void 
GlideComputerTaskEvents::task_finish()
{
  InputEvents::processGlideComputer(GCE_TASK_FINISH);
  m_computer->OnFinishTask();
}

void
GlideComputerTaskEvents::transition_flight_mode(const bool is_final)
{
  if (is_final) {
    InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE);
  } else {
    InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
  }
}

