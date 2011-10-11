/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "TaskEventsPrint.hpp"
#include "Task/Tasks/BaseTask/TaskWaypoint.hpp"

#include <stdio.h>

void
TaskEventsPrint::EnterTransition(const TaskWaypoint& tp)
{
  if (verbose)
    printf("#- entered sector\n");
}

void
TaskEventsPrint::AlternateTransition()
{
  if (verbose)
    printf("#- alternate changed\n");
}

void
TaskEventsPrint::ExitTransition(const TaskWaypoint &tp)
{
  if (verbose)
    printf("#- exited sector\n");
}

void
TaskEventsPrint::ActiveChanged(const TaskWaypoint &tp)
{
  if (verbose)
    printf("#- active changed to wp %d\n", tp.GetWaypoint().id);
}

void
TaskEventsPrint::ConstructionError(const char* error)
{
  if (verbose) {
    printf("#Task construction error: ");
    printf("#%s", error);
    printf("\n");
  }
}

void
TaskEventsPrint::StartSpeedWarning()
{
  if (verbose)
    printf("#- warning speed excessive\n");
}

void
TaskEventsPrint::RequestArm(const TaskWaypoint &tp)
{
  if (verbose)
    printf("#- ready to advance\n");
}

void
TaskEventsPrint::TaskStart()
{
  if (verbose)
    printf("#- task started\n");
}

void
TaskEventsPrint::TaskFinish()
{
  if (verbose)
    printf("#- task finished\n");
}

void
TaskEventsPrint::ActiveAdvanced(const TaskWaypoint &tp, const int i)
{
  if (verbose)
    printf("#- advance to sector %d\n", i);
}

void
TaskEventsPrint::FlightModeTransition(const bool is_final)
{
  if (verbose) {
    if (is_final)
      printf("#- flight mode final glide \n");
    else
      printf("#- flight mode cruise \n");
  }
}
