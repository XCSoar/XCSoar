// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Engine/Task/ObservationZones/KeyholeZone.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"
#include "Engine/Task/ObservationZones/SectorZone.hpp"
#include "Engine/Task/ObservationZones/SymmetricSectorZone.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Engine/Task/Ordered/Points/ASTPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Task/CupTaskQR.hpp"
#include "Task/LoadFile.hpp"
#include "Task/SaveFile.hpp"
#include "Task/XCSoarTaskQR.hpp"
#include "TestUtil.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

static TaskBehaviour task_behaviour;
static OrderedTaskSettings ordered_task_settings;
static constexpr Path task_path{"output/results/Test-Task.tsk"};

static constexpr GeoPoint
MakeGeoPoint(double longitude, double latitude) noexcept
{
  return {Angle::Degrees(longitude), Angle::Degrees(latitude)};
}

static Waypoint
MakeWaypoint(Waypoint wp,
             double altitude,
             std::string name,
             unsigned id) noexcept
{
  wp.name             = name;
  wp.id               = id;
  wp.elevation        = altitude;
  wp.elevation_source = Waypoint::ElevationSource::FILE;
  return wp;
}

static Waypoint
MakeWaypoint(double longitude,
             double latitude,
             double altitude,
             std::string name,
             unsigned id) noexcept
{
  return MakeWaypoint(
    Waypoint(MakeGeoPoint(longitude, latitude)), altitude, name, id);
}

template <typename... Args>
static WaypointPtr
MakeWaypointPtr(Args &&...args) noexcept
{
  return WaypointPtr(new Waypoint(MakeWaypoint(std::forward<Args>(args)...)));
}

static const auto wp1_str = "wp-01";
static const auto wp2_str = "wp-02";
static const auto wp3_str = "wp-03";

static const auto wp1 = MakeWaypointPtr(0, 45, 50, wp1_str, 100);
static const auto wp2 = MakeWaypointPtr(0, 45.3, 50, wp2_str, 102);
static const auto wp3 = MakeWaypointPtr(0, 46, 50, wp3_str, 103);

static void
TestTaskSave()
{
  OrderedTask task_saved(task_behaviour);

  /* a non-default task rule, to check that the task carries it
     instead of falling back to the TaskBehaviour defaults on load */
  OrderedTaskSettings settings = ordered_task_settings;
  settings.navigate_nearest    = true;
  task_saved.SetOrderedTaskSettings(settings);

  task_saved.Append(
    StartPoint(std::make_unique<CylinderZone>(wp1->location, 500),
               WaypointPtr(wp1),
               task_behaviour,
               ordered_task_settings.start_constraints));
  task_saved.Append(
    AATPoint(std::make_unique<CylinderZone>(wp2->location, 10000),
             WaypointPtr(wp2),
             task_behaviour));
  task_saved.Append(
    FinishPoint(std::make_unique<CylinderZone>(wp3->location, 500),
                WaypointPtr(wp3),
                task_behaviour,
                ordered_task_settings.finish_constraints));
  task_saved.SetActiveTaskPoint(1);
  task_saved.UpdateGeometry();
  SaveTask(task_path, task_saved);

  std::unique_ptr<OrderedTask> task_loaded =
    LoadTask(task_path, task_behaviour);
  ok1(task_loaded);

  ok1(task_loaded.get()->IsValidIndex(0));
  ok1(task_loaded.get()->IsValidIndex(1));
  ok1(task_loaded.get()->IsValidIndex(2));

  ok1(StringIsEqual(
    task_loaded.get()->GetTaskPoint(0).GetWaypoint().name.c_str(), wp1_str));
  ok1(StringIsEqual(
    task_loaded.get()->GetTaskPoint(1).GetWaypoint().name.c_str(), wp2_str));
  ok1(StringIsEqual(
    task_loaded.get()->GetTaskPoint(2).GetWaypoint().name.c_str(), wp3_str));

  ok1(task_loaded.get()->GetTaskPoint(0).GetWaypoint() == *wp1);
  ok1(task_loaded.get()->GetTaskPoint(1).GetWaypoint() == *wp2);
  ok1(task_loaded.get()->GetTaskPoint(2).GetWaypoint() == *wp3);

  ok1(task_loaded.get()->GetOrderedTaskSettings().navigate_nearest);
}

static void
TestTaskQR()
{
  OrderedTask task(task_behaviour);
  task.SetFactory(TaskFactoryType::RACING);

  OrderedTaskSettings settings = ordered_task_settings;
  settings.navigate_nearest    = true;
  task.SetOrderedTaskSettings(settings);

  task.Append(StartPoint(std::make_unique<LineSectorZone>(wp1->location, 2000),
                         WaypointPtr(wp1),
                         task_behaviour,
                         ordered_task_settings.start_constraints));
  task.Append(ASTPoint(std::make_unique<CylinderZone>(wp2->location, 500),
                       WaypointPtr(wp2),
                       task_behaviour));
  task.Append(
    FinishPoint(std::make_unique<LineSectorZone>(wp3->location, 4000),
                WaypointPtr(wp3),
                task_behaviour,
                ordered_task_settings.finish_constraints));

  const auto text = EncodeXCSoarTaskQRText(task);
  const bool have = text.has_value();
  ok1(have);
  ok1(have && text->starts_with(XCSOAR_TASK_QR_PREFIX));

  std::unique_ptr<OrderedTask> loaded;
  if (have)
    loaded = DecodeXCSoarTaskQRText(text->substr(XCSOAR_TASK_QR_PREFIX.size()),
                                    task_behaviour);

  ok1(loaded != nullptr);
  ok1(loaded && loaded->GetFactoryType() == TaskFactoryType::RACING);
  ok1(loaded && loaded->TaskSize() == 3);
  ok1(loaded && loaded->GetPoint(0).GetObservationZone().GetShape() ==
                  ObservationZone::Shape::LINE);
  ok1(loaded && equals(static_cast<const LineSectorZone &>(
                         loaded->GetPoint(0).GetObservationZone())
                         .GetLength(),
                       2000));
  ok1(loaded && loaded->GetPoint(1).GetObservationZone().GetShape() ==
                  ObservationZone::Shape::CYLINDER);
  ok1(loaded && equals(static_cast<const CylinderZone &>(
                         loaded->GetPoint(1).GetObservationZone())
                         .GetRadius(),
                       500));
  ok1(loaded && loaded->GetPoint(2).GetObservationZone().GetShape() ==
                  ObservationZone::Shape::LINE);
  ok1(loaded && equals(static_cast<const LineSectorZone &>(
                         loaded->GetPoint(2).GetObservationZone())
                         .GetLength(),
                       4000));
  ok1(loaded && loaded->GetOrderedTaskSettings().navigate_nearest);
  ok1(loaded &&
      StringIsEqual(loaded->GetTaskPoint(0).GetWaypoint().name.c_str(),
                    wp1_str));
}

static void
TestCupTaskQR()
{
  OrderedTask task(task_behaviour);
  task.SetFactory(TaskFactoryType::RACING);
  task.Append(StartPoint(std::make_unique<LineSectorZone>(wp1->location, 2000),
                         WaypointPtr(wp1),
                         task_behaviour,
                         ordered_task_settings.start_constraints));
  task.Append(ASTPoint(SymmetricSectorZone::CreateSymmetricCircularSectorZone(
                         wp2->location, 5000, Angle::Degrees(60)),
                       WaypointPtr(wp2),
                       task_behaviour));
  task.Append(
    FinishPoint(std::make_unique<LineSectorZone>(wp3->location, 4000),
                WaypointPtr(wp3),
                task_behaviour,
                ordered_task_settings.finish_constraints));

  const auto text = EncodeCupTaskQRText(task);
  const bool have = text.has_value();
  ok1(have);
  ok1(have && text->starts_with(CUP_TASK_QR_PREFIX));

  std::unique_ptr<OrderedTask> loaded;
  if (have)
    loaded = DecodeCupTaskQRText(text->substr(CUP_TASK_QR_PREFIX.size()),
                                 task_behaviour);

  ok1(loaded != nullptr);
  ok1(loaded && loaded->GetFactoryType() == TaskFactoryType::RACING);
  ok1(loaded && loaded->TaskSize() == 3);
  ok1(loaded && loaded->GetPoint(0).GetObservationZone().GetShape() ==
                  ObservationZone::Shape::LINE);
  ok1(loaded && equals(static_cast<const LineSectorZone &>(
                         loaded->GetPoint(0).GetObservationZone())
                         .GetLength(),
                       2000));
  ok1(loaded && loaded->GetPoint(1).GetObservationZone().GetShape() ==
                  ObservationZone::Shape::SYMMETRIC_SECTOR);
  ok1(loaded && equals(static_cast<const CylinderZone &>(
                         loaded->GetPoint(1).GetObservationZone())
                         .GetRadius(),
                       5000));
  ok1(loaded && equals(static_cast<const SymmetricSectorZone &>(
                         loaded->GetPoint(1).GetObservationZone())
                         .GetSectorAngle(),
                       60));
  ok1(loaded && loaded->GetPoint(2).GetObservationZone().GetShape() ==
                  ObservationZone::Shape::LINE);
  ok1(loaded && equals(static_cast<const LineSectorZone &>(
                         loaded->GetPoint(2).GetObservationZone())
                         .GetLength(),
                       4000));
  ok1(loaded &&
      StringIsEqual(loaded->GetTaskPoint(0).GetWaypoint().name.c_str(),
                    wp1_str));

  OrderedTask fai(task_behaviour);
  fai.SetFactory(TaskFactoryType::RACING);
  fai.Append(StartPoint(std::make_unique<LineSectorZone>(wp1->location, 2000),
                        WaypointPtr(wp1),
                        task_behaviour,
                        ordered_task_settings.start_constraints));
  fai.Append(
    ASTPoint(SymmetricSectorZone::CreateFAISectorZone(wp2->location, true),
             WaypointPtr(wp2),
             task_behaviour));
  fai.Append(FinishPoint(std::make_unique<LineSectorZone>(wp3->location, 4000),
                         WaypointPtr(wp3),
                         task_behaviour,
                         ordered_task_settings.finish_constraints));
  const auto fai_text = EncodeCupTaskQRText(fai);
  ok1(fai_text.has_value());
  std::unique_ptr<OrderedTask> fai_loaded;
  if (fai_text)
    fai_loaded = DecodeCupTaskQRText(
      fai_text->substr(CUP_TASK_QR_PREFIX.size()), task_behaviour);
  ok1(fai_loaded && fai_loaded->GetPoint(1).GetObservationZone().GetShape() ==
                      ObservationZone::Shape::FAI_SECTOR);

  OrderedTask aat(task_behaviour);
  aat.SetFactory(TaskFactoryType::AAT);
  OrderedTaskSettings settings = ordered_task_settings;
  settings.aat_min_time        = std::chrono::minutes(90);
  aat.SetOrderedTaskSettings(settings);
  aat.Append(StartPoint(std::make_unique<CylinderZone>(wp1->location, 500),
                        WaypointPtr(wp1),
                        task_behaviour,
                        ordered_task_settings.start_constraints));
  aat.Append(
    AATPoint(std::make_unique<SectorZone>(
               wp2->location, 10000, Angle::Degrees(10), Angle::Degrees(80)),
             WaypointPtr(wp2),
             task_behaviour));
  aat.Append(FinishPoint(std::make_unique<CylinderZone>(wp3->location, 500),
                         WaypointPtr(wp3),
                         task_behaviour,
                         ordered_task_settings.finish_constraints));
  const auto aat_text = EncodeCupTaskQRText(aat);
  ok1(aat_text.has_value());
  std::unique_ptr<OrderedTask> aat_loaded;
  if (aat_text)
    aat_loaded = DecodeCupTaskQRText(
      aat_text->substr(CUP_TASK_QR_PREFIX.size()), task_behaviour);
  ok1(aat_loaded && aat_loaded->GetFactoryType() == TaskFactoryType::AAT);
  ok1(aat_loaded &&
      aat_loaded->GetOrderedTaskSettings().aat_min_time.count() == 90u * 60u);
  ok1(aat_loaded && aat_loaded->GetPoint(1).GetObservationZone().GetShape() ==
                      ObservationZone::Shape::SECTOR);
  ok1(aat_loaded && equals(static_cast<const SectorZone &>(
                             aat_loaded->GetPoint(1).GetObservationZone())
                             .GetRadius(),
                           10000));
  ok1(aat_loaded && equals(static_cast<const SectorZone &>(
                             aat_loaded->GetPoint(1).GetObservationZone())
                             .GetStartRadial(),
                           10));
  ok1(aat_loaded && equals(static_cast<const SectorZone &>(
                             aat_loaded->GetPoint(1).GetObservationZone())
                             .GetEndRadial(),
                           80));

  OrderedTask keyhole(task_behaviour);
  keyhole.SetFactory(TaskFactoryType::RACING);
  keyhole.Append(
    StartPoint(std::make_unique<LineSectorZone>(wp1->location, 2000),
               WaypointPtr(wp1),
               task_behaviour,
               ordered_task_settings.start_constraints));
  keyhole.Append(ASTPoint(KeyholeZone::CreateDAeCKeyholeZone(wp2->location),
                          WaypointPtr(wp2),
                          task_behaviour));
  keyhole.Append(
    FinishPoint(std::make_unique<LineSectorZone>(wp3->location, 4000),
                WaypointPtr(wp3),
                task_behaviour,
                ordered_task_settings.finish_constraints));
  ok1(!EncodeCupTaskQRText(keyhole));
}

static void
TestAll()
{
  TestTaskSave();
  TestTaskQR();
  TestCupTaskQR();
}

int
main()
{
  Directory::Create(Path{"output/results"});

  plan_tests(11 + 13 + 23);
  task_behaviour.SetDefaults();
  ordered_task_settings.SetDefaults();
  TestAll();
  return exit_status();
}
