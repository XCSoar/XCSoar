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

#include "OS/Args.hpp"
#include "DebugReplay.hpp"
#include "Task/TaskFile.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "NMEA/Aircraft.hpp"
#include "Formatter/TimeFormatter.hpp"

#include <stdio.h>
#include <stdlib.h>

static void
Run(DebugReplay &replay, OrderedTask &task, const GlidePolar &glide_polar)
{
  Validity last_location_available;
  last_location_available.Clear();

  AircraftState last_as;
  bool last_as_valid = false;
  bool task_finished = false;

  unsigned active_taskpoint_index(-1);

  char time_buffer[32];

  while (replay.Next()) {
    const MoreData &basic = replay.Basic();
    const DerivedInfo &calculated = replay.Calculated();

    if (!basic.location_available) {
      last_location_available.Clear();
      continue;
    }

    const AircraftState current_as = ToAircraftState(basic, calculated);

    if (!last_location_available) {
      last_as = current_as;
      last_as_valid = true;
      last_location_available = basic.location_available;
      continue;
    }

    if (!basic.location_available.Modified(last_location_available))
      continue;

    if (!last_as_valid) {
      last_as = current_as;
      last_as_valid = true;
      last_location_available = basic.location_available;
      continue;
    }

    task.Update(current_as, last_as, glide_polar);
    task.UpdateIdle(current_as, glide_polar);
    task.SetTaskAdvance().SetArmed(true);

    if (task.GetActiveIndex() != active_taskpoint_index) {
      active_taskpoint_index = task.GetActiveIndex();

      FormatISO8601(time_buffer, basic.date_time_utc);
      printf("%s active_taskpoint_index=%u\n",
             time_buffer, active_taskpoint_index);
    }

    const TaskStats &task_stats = task.GetStats();
    if (task_finished != task_stats.task_finished) {
      task_finished = true;
      FormatISO8601(time_buffer, basic.date_time_utc);
      printf("%s task finished\n", time_buffer);
    }

    last_as = current_as;
    last_as_valid = true;
  }

  const TaskStats &task_stats = task.GetStats();

  printf("task_started=%d task_finished=%d\n",
         task_stats.start.task_started,
         task_stats.task_finished);

  printf("task elapsed %ds\n", (int)task_stats.total.time_elapsed);
  printf("task speed %1.2f kph\n",
         double(task_stats.total.travelled.GetSpeed() * 3.6));
  printf("travelled distance %1.3f km\n",
         double(task_stats.total.travelled.GetDistance() / 1000));
  printf("scored distance %1.3f km\n",
         double(task_stats.distance_scored / 1000));
  if (task_stats.total.time_elapsed > 0)
    printf("scored speed %1.2f kph\n",
           double(task_stats.distance_scored
                  / task_stats.total.time_elapsed * 3.6));
}

int main(int argc, char **argv)
{
  Args args(argc, argv, "TASKFILE REPLAYFILE");
  const auto task_path = args.ExpectNextPath();
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  OrderedTask *task = TaskFile::GetTask(task_path, task_behaviour,
                                        NULL, 0);
  if (task == NULL) {
    fprintf(stderr, "Failed to load task\n");
    return EXIT_FAILURE;
  }

  task->UpdateGeometry();

  const GlidePolar glide_polar(1);

  Run(*replay, *task, glide_polar);
  delete task;
  delete replay;

  return EXIT_SUCCESS;
}
