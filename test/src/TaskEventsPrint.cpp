/* Copyright_License {

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

#include "TaskEventsPrint.hpp"
#include "Task/Points/TaskWaypoint.hpp"

#include <stdio.h>

void
TaskEventsPrint::EnterTransition(const TaskWaypoint& tp)
{
  if (verbose)
    printf("#- entered sector\n");
}

void
TaskEventsPrint::ExitTransition(const TaskWaypoint &tp)
{
  if (verbose)
    printf("#- exited sector\n");
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
