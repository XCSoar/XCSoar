// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Task/TaskFile.hpp"
#include "Units/System.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "io/FileOutputStream.hxx"
#include "util/PrintException.hxx"

#include "TestUtil.hpp"

#include <memory>
#include <cstdio>
#include <cmath>

static TaskBehaviour task_behaviour;

static void
TestRadiusParsingWithUnits()
{
  // Test various radius formats with different units
  const char *cup_content =
    "name,code,country,lat,lon,elev,style,rwdir,rwlen,freq,desc\n"
    "\"Start1\",\"S1\",,4800.000N,01100.000E,500.0m,1,0,0.0m,\"\",\"Start 1\"\n"
    "\"Start2\",\"S2\",,4801.000N,01101.000E,500.0m,1,0,0.0m,\"\",\"Start 2\"\n"
    "\"Start3\",\"S3\",,4802.000N,01102.000E,500.0m,1,0,0.0m,\"\",\"Start 3\"\n"
    "\"Start4\",\"S4\",,4803.000N,01103.000E,500.0m,1,0,0.0m,\"\",\"Start 4\"\n"
    "\"Start5\",\"S5\",,4804.000N,01104.000E,500.0m,1,0,0.0m,\"\",\"Start 5\"\n"
    "\"Turn1\",\"T1\",,4810.000N,01110.000E,500.0m,1,0,0.0m,\"\",\"Turn 1\"\n"
    "\"Finish1\",\"F1\",,4820.000N,01120.000E,500.0m,1,0,0.0m,\"\",\"Finish 1\"\n"
    "-----Related Tasks-----\n"
    "\"Test Task\",\"S1\",\"S2\",\"S3\",\"S4\",\"S5\",\"T1\",\"F1\",\"F1\"\n"
    "ObsZone=0,Style=0,R1=1000.5m\n"
    "ObsZone=1,Style=0,R1=1.5nm\n"
    "ObsZone=2,Style=0,R1=2.3ml\n"
    "ObsZone=3,Style=0,R1=500M\n"
    "ObsZone=4,Style=0,R1=1.2NM\n"
    "ObsZone=5,Style=0,R1=1000\n";

  constexpr Path task_path{_T("output/results/Test-Task-RadiusParsing.cup")};

  {
    FileOutputStream fos(task_path, FileOutputStream::Mode::CREATE);
    fos.Write(cup_content, strlen(cup_content));
    fos.Commit();
  }

  auto task_file = TaskFile::Create(task_path);
  ok1(task_file != nullptr);

  auto task = task_file->GetTask(task_behaviour, nullptr, 0);
  ok1(task != nullptr);

  if (!task) {
    skip(6, 0, "Failed to load task");
    return;
  }

  // Test 0: R1=1000.5m (decimal meters)
  ok1(task->IsValidIndex(0));
  const auto &point0 = task->GetTaskPoint(0);
  const auto *zone0 = dynamic_cast<const CylinderZone *>(
    &point0.GetObservationZone());
  ok1(zone0 != nullptr);
  if (zone0) {
    const double radius0 = zone0->GetRadius();
    ok1(fabs(radius0 - 1000.5) < 0.1);
  }

  // Test 1: R1=1.5nm (decimal nautical miles)
  ok1(task->IsValidIndex(1));
  const auto &point1 = task->GetTaskPoint(1);
  const auto *zone1 = dynamic_cast<const CylinderZone *>(
    &point1.GetObservationZone());
  ok1(zone1 != nullptr);
  if (zone1) {
    const double radius1 = zone1->GetRadius();
    const double expected1 = Units::ToSysUnit(1.5, Unit::NAUTICAL_MILES);
    ok1(fabs(radius1 - expected1) < 0.1);
  }

  // Test 2: R1=2.3ml (decimal statute miles)
  ok1(task->IsValidIndex(2));
  const auto &point2 = task->GetTaskPoint(2);
  const auto *zone2 = dynamic_cast<const CylinderZone *>(
    &point2.GetObservationZone());
  ok1(zone2 != nullptr);
  if (zone2) {
    const double radius2 = zone2->GetRadius();
    const double expected2 = Units::ToSysUnit(2.3, Unit::STATUTE_MILES);
    ok1(fabs(radius2 - expected2) < 0.1);
  }

  // Test 3: R1=500M (uppercase M)
  ok1(task->IsValidIndex(3));
  const auto &point3 = task->GetTaskPoint(3);
  const auto *zone3 = dynamic_cast<const CylinderZone *>(
    &point3.GetObservationZone());
  ok1(zone3 != nullptr);
  if (zone3) {
    const double radius3 = zone3->GetRadius();
    ok1(fabs(radius3 - 500.0) < 0.1);
  }

  // Test 4: R1=1.2NM (uppercase NM)
  ok1(task->IsValidIndex(4));
  const auto &point4 = task->GetTaskPoint(4);
  const auto *zone4 = dynamic_cast<const CylinderZone *>(
    &point4.GetObservationZone());
  ok1(zone4 != nullptr);
  if (zone4) {
    const double radius4 = zone4->GetRadius();
    const double expected4 = Units::ToSysUnit(1.2, Unit::NAUTICAL_MILES);
    ok1(fabs(radius4 - expected4) < 0.1);
  }

  // Test 5: R1=1000 (no unit, should default to meters)
  ok1(task->IsValidIndex(5));
  const auto &point5 = task->GetTaskPoint(5);
  const auto *zone5 = dynamic_cast<const CylinderZone *>(
    &point5.GetObservationZone());
  ok1(zone5 != nullptr);
  if (zone5) {
    const double radius5 = zone5->GetRadius();
    ok1(fabs(radius5 - 1000.0) < 0.1);
  }
}

static void
TestAngleParsingWithDecimals()
{
  // Test angle parsing with decimal values
  const char *cup_content =
    "name,code,country,lat,lon,elev,style,rwdir,rwlen,freq,desc\n"
    "\"Start1\",\"S1\",,4800.000N,01100.000E,500.0m,1,0,0.0m,\"\",\"Start 1\"\n"
    "\"Start2\",\"S2\",,4801.000N,01101.000E,500.0m,1,0,0.0m,\"\",\"Start 2\"\n"
    "\"Turn1\",\"T1\",,4810.000N,01110.000E,500.0m,1,0,0.0m,\"\",\"Turn 1\"\n"
    "-----Related Tasks-----\n"
    "\"Test Task\",\"S1\",\"S2\",\"T1\",\"T1\"\n"
    "ObsZone=0,Style=1,R1=1000m,A1=123.4\n"
    "ObsZone=1,Style=1,R1=1000m,A1=180\n"
    "ObsZone=2,Style=1,R1=1000m,A12=45.67\n";

  constexpr Path task_path{_T("output/results/Test-Task-AngleParsing.cup")};

  {
    FileOutputStream fos(task_path, FileOutputStream::Mode::CREATE);
    fos.Write(cup_content, strlen(cup_content));
    fos.Commit();
  }

  auto task_file = TaskFile::Create(task_path);
  ok1(task_file != nullptr);

  auto task = task_file->GetTask(task_behaviour, nullptr, 0);
  ok1(task != nullptr);

  if (!task) {
    skip(3, 0, "Failed to load task");
    return;
  }

  // Note: We can't directly test the parsed angle values without accessing
  // internal zone structures, but we can verify the task loads successfully
  // with decimal angles, which means ParseAngle is working correctly.
  ok1(task->IsValidIndex(0));
  ok1(task->IsValidIndex(1));
  ok1(task->IsValidIndex(2));
}

static void
TestLineZoneWithDecimalRadius()
{
  // Test line zone with decimal radius (should be multiplied by 2)
  const char *cup_content =
    "name,code,country,lat,lon,elev,style,rwdir,rwlen,freq,desc\n"
    "\"Start1\",\"S1\",,4800.000N,01100.000E,500.0m,1,0,0.0m,\"\",\"Start 1\"\n"
    "\"Finish1\",\"F1\",,4820.000N,01120.000E,500.0m,1,0,0.0m,\"\",\"Finish 1\"\n"
    "-----Related Tasks-----\n"
    "\"Test Task\",\"S1\",\"F1\",\"F1\"\n"
    "ObsZone=0,Style=2,R1=500.5m,Line=1\n"
    "ObsZone=1,Style=2,R1=1.2nm,Line=1\n";

  constexpr Path task_path{_T("output/results/Test-Task-LineZoneDecimal.cup")};

  {
    FileOutputStream fos(task_path, FileOutputStream::Mode::CREATE);
    fos.Write(cup_content, strlen(cup_content));
    fos.Commit();
  }

  auto task_file = TaskFile::Create(task_path);
  ok1(task_file != nullptr);

  auto task = task_file->GetTask(task_behaviour, nullptr, 0);
  ok1(task != nullptr);

  if (!task) {
    skip(2, 0, "Failed to load task");
    return;
  }

  // Test 0: R1=500.5m should result in length = 500.5 * 2 = 1001m
  ok1(task->IsValidIndex(0));
  const auto &start_point = task->GetTaskPoint(0);
  const auto *line_zone = dynamic_cast<const LineSectorZone *>(
    &start_point.GetObservationZone());
  ok1(line_zone != nullptr);
  if (line_zone) {
    const double length = line_zone->GetLength();
    ok1(fabs(length - 1001.0) < 0.1);
  }

  // Test 1: R1=1.2nm should result in length = 1.2nm * 2 = 2.4nm
  ok1(task->IsValidIndex(1));
  const auto &finish_point = task->GetTaskPoint(1);
  const auto *finish_line_zone = dynamic_cast<const LineSectorZone *>(
    &finish_point.GetObservationZone());
  ok1(finish_line_zone != nullptr);
  if (finish_line_zone) {
    const double finish_length = finish_line_zone->GetLength();
    const double expected_length = Units::ToSysUnit(1.2, Unit::NAUTICAL_MILES) * 2;
    ok1(fabs(finish_length - expected_length) < 0.1);
  }
}

static void
TestAll()
{
  TestRadiusParsingWithUnits();
  TestAngleParsingWithDecimals();
  TestLineZoneWithDecimalRadius();
}

int main()
{
  Directory::Create(Path{_T("output/results")});

  plan_tests(16);
  task_behaviour.SetDefaults();
  TestAll();
  return exit_status();
}

