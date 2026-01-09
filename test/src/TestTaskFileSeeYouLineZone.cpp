// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Task/TaskFile.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "io/FileOutputStream.hpp"
#include "util/PrintException.hxx"

#include "TestUtil.hpp"

#include <memory>
#include <cstdio>

static TaskBehaviour task_behaviour;
static constexpr Path task_path{_T("output/results/Test-Task-LineZone.cup")};

static void
TestLineZoneRadius()
{
  // Create a minimal CUP file with a task that has a line zone
  // Format: waypoints, then "-----Related Tasks-----", then task definition
  const char *cup_content =
    "name,code,country,lat,lon,elev,style,rwdir,rwlen,freq,desc\n"
    "\"StartWP\",\"START\",,4800.000N,01100.000E,500.0m,1,0,0.0m,\"\",\"Start waypoint\"\n"
    "\"TurnWP\",\"TURN\",,4810.000N,01110.000E,500.0m,1,0,0.0m,\"\",\"Turn waypoint\"\n"
    "\"FinishWP\",\"FINISH\",,4820.000N,01120.000E,500.0m,1,0,0.0m,\"\",\"Finish waypoint\"\n"
    "-----Related Tasks-----\n"
    "\"Test Task\",\"START\",\"TURN\",\"FINISH\",\"FINISH\"\n"
    "ObsZone=0,Style=2,R1=500m,Line=1\n"
    "ObsZone=1,Style=0,R1=1000m\n"
    "ObsZone=2,Style=2,R1=300m,Line=1\n";

  // Write CUP file
  {
    FileOutputStream fos(task_path, FileOutputStream::Mode::CREATE);
    fos.Write(cup_content, strlen(cup_content));
    fos.Commit();
  }

  // Load task from CUP file
  auto task_file = TaskFile::Create(task_path);
  ok1(task_file != nullptr);

  auto task = task_file->GetTask(task_behaviour, nullptr, 0);
  ok1(task != nullptr);

  if (!task) {
    skip(3, 0, "Failed to load task");
    return;
  }

  // Verify start point (index 0) has a LineSectorZone with correct length
  ok1(task->IsValidIndex(0));
  const auto &start_point = task->GetTaskPoint(0);
  const auto *line_zone = dynamic_cast<const LineSectorZone *>(
    &start_point.GetObservationZone());
  ok1(line_zone != nullptr);

  if (line_zone) {
    // R1=500m should result in length = 500 * 2 = 1000m
    const double length = line_zone->GetLength();
    ok1(fabs(length - 1000.0) < 0.1);
  } else {
    skip(1, 0, "Start point is not a LineSectorZone");
  }

  // Verify finish point (index 2) also has a LineSectorZone with correct length
  ok1(task->IsValidIndex(2));
  const auto &finish_point = task->GetTaskPoint(2);
  const auto *finish_line_zone = dynamic_cast<const LineSectorZone *>(
    &finish_point.GetObservationZone());
  ok1(finish_line_zone != nullptr);

  if (finish_line_zone) {
    // R1=300m should result in length = 300 * 2 = 600m
    const double finish_length = finish_line_zone->GetLength();
    ok1(fabs(finish_length - 600.0) < 0.1);
  } else {
    skip(1, 0, "Finish point is not a LineSectorZone");
  }
}

static void
TestLineZoneRadiusWithoutUnit()
{
  // Test with R1 without 'm' suffix
  const char *cup_content =
    "name,code,country,lat,lon,elev,style,rwdir,rwlen,freq,desc\n"
    "\"StartWP\",\"START\",,4800.000N,01100.000E,500.0m,1,0,0.0m,\"\",\"Start waypoint\"\n"
    "\"FinishWP\",\"FINISH\",,4820.000N,01120.000E,500.0m,1,0,0.0m,\"\",\"Finish waypoint\"\n"
    "-----Related Tasks-----\n"
    "\"Test Task\",\"START\",\"FINISH\",\"FINISH\"\n"
    "ObsZone=0,Style=2,R1=1300,Line=1\n";

  constexpr Path task_path2{_T("output/results/Test-Task-LineZone2.cup")};

  {
    FileOutputStream fos(task_path2, FileOutputStream::Mode::CREATE);
    fos.Write(cup_content, strlen(cup_content));
    fos.Commit();
  }

  auto task_file = TaskFile::Create(task_path2);
  ok1(task_file != nullptr);

  auto task = task_file->GetTask(task_behaviour, nullptr, 0);
  ok1(task != nullptr);

  if (!task) {
    skip(2, 0, "Failed to load task");
    return;
  }

  ok1(task->IsValidIndex(0));
  const auto &start_point = task->GetTaskPoint(0);
  const auto *line_zone = dynamic_cast<const LineSectorZone *>(
    &start_point.GetObservationZone());
  ok1(line_zone != nullptr);

  if (line_zone) {
    // R1=1300 should result in length = 1300 * 2 = 2600m
    const double length = line_zone->GetLength();
    ok1(fabs(length - 2600.0) < 0.1);
  } else {
    skip(1, 0, "Start point is not a LineSectorZone");
  }
}

static void
TestAll()
{
  TestLineZoneRadius();
  TestLineZoneRadiusWithoutUnit();
}

int main()
{
  Directory::Create(Path{_T("output/results")});

  plan_tests(10);
  task_behaviour.SetDefaults();
  TestAll();
  return exit_status();
}

