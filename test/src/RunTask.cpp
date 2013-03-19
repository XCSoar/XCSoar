/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "NMEA/Aircraft.hpp"
#include "Formatter/TimeFormatter.hpp"

#include <stdio.h>
#include <stdlib.h>

static void
Run(DebugReplay &replay, TaskManager &task_manager)
{
  if (!replay.Next())
    return;

  MoreData last_basic;
  last_basic = replay.Basic();

  DerivedInfo last_calculated;
  last_calculated = replay.Calculated();

  unsigned active_taskpoint_index(-1);

  char time_buffer[32];

  while (replay.Next()) {
    const MoreData &basic = replay.Basic();
    const DerivedInfo &calculated = replay.Calculated();

    if (!basic.HasTimeAdvancedSince(last_basic) ||
        !basic.location_available)
      continue;

    const AircraftState current_as = ToAircraftState(basic, calculated);
    const AircraftState last_as = ToAircraftState(last_basic,
                                                  last_calculated);
    task_manager.Update(current_as, last_as);
    task_manager.UpdateIdle(current_as);
    task_manager.SetTaskAdvance().SetArmed(true);

    const CommonStats &common_stats = task_manager.GetCommonStats();
    if (common_stats.active_taskpoint_index != active_taskpoint_index) {
      active_taskpoint_index = common_stats.active_taskpoint_index;

      FormatISO8601(time_buffer, basic.date_time_utc);
      printf("%s active_taskpoint_index=%u\n",
             time_buffer, active_taskpoint_index);
    }

    last_basic = basic;
    last_calculated = calculated;
  }

  const TaskStats &task_stats = task_manager.GetOrderedTask().GetStats();

  printf("task_started=%d task_finished=%d\n",
         task_stats.task_started, task_stats.task_finished);

  printf("task elapsed %ds\n", (int)task_stats.total.time_elapsed);
  printf("task speed %1.1f kph\n",
         double(task_stats.total.travelled.GetSpeed() * fixed(3.6)));
  printf("travelled distance %1.1f km\n",
         double(task_stats.total.travelled.GetDistance() / 1000));
  printf("scored distance %1.1f km\n",
         double(task_stats.distance_scored / 1000));
  if (positive(task_stats.total.time_elapsed))
    printf("scored speed %1.1f kph\n",
           double(task_stats.distance_scored
                  / task_stats.total.time_elapsed * fixed(3.6)));
}

int main(int argc, char **argv)
{
  Args args(argc, argv, "TASKFILE REPLAYFILE");
  tstring task_path = args.ExpectNextT();
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  Waypoints way_points;

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  OrderedTask *task = TaskFile::GetTask(task_path.c_str(), task_behaviour,
                                        NULL, 0);
  if (task == NULL) {
    fprintf(stderr, "Failed to load task\n");
    return EXIT_FAILURE;
  }

  TaskManager task_manager(task_behaviour, way_points);
  task_manager.SetGlidePolar(GlidePolar(fixed(1)));

  if (!task_manager.Commit(*task)) {
    fprintf(stderr, "Failed to commit task\n");
    return EXIT_FAILURE;
  }

  delete task;

  Run(*replay, task_manager);
  delete replay;

  return EXIT_SUCCESS;
}
