/*
Copyright_License {

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

#include "GlideComputerInterface.hpp"
#include "GlideComputer.hpp"
#include "Input/InputQueue.hpp"

void
GlideComputerTaskEvents::SetComputer(GlideComputer &_computer)
{
  computer = &_computer;
}

void
GlideComputerTaskEvents::EnterTransition(const TaskWaypoint &tp)
{
  computer->OnTransitionEnter();
}

void
GlideComputerTaskEvents::RequestArm(const TaskWaypoint &tp)
{
  InputEvents::processGlideComputer(GCE_ARM_READY);
}

void
GlideComputerTaskEvents::ActiveAdvanced(const TaskWaypoint &tp, const int i)
{
  InputEvents::processGlideComputer(GCE_TASK_NEXTWAYPOINT);
}

void
GlideComputerTaskEvents::TaskStart()
{
  InputEvents::processGlideComputer(GCE_TASK_START);
  computer->OnStartTask();
}

void
GlideComputerTaskEvents::TaskFinish()
{
  InputEvents::processGlideComputer(GCE_TASK_FINISH);
}
