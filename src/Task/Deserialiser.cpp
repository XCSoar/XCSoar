// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Deserialiser.hpp"
#include "Task/Ordered/Settings.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Task/Ordered/Points/StartPoint.hpp"
#include "Task/Ordered/Points/FinishPoint.hpp"
#include "Task/Ordered/Points/AATPoint.hpp"
#include "Task/Ordered/Points/ASTPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "XML/DataNode.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#include <memory>

static void
Deserialise(GeoPoint &data, const ConstDataNode &node)
{
  node.GetAttribute("longitude", data.longitude);
  node.GetAttribute("latitude", data.latitude);
}

static WaypointPtr
DeserialiseWaypoint(const ConstDataNode &node, const Waypoints *waypoints)
{
  auto loc_node = node.GetChildNamed("Location");
  if (!loc_node)
    return nullptr;

  GeoPoint loc;
  Deserialise(loc, *loc_node);

  const char *name = node.GetAttribute("name");
  if (name == nullptr)
    // Turnpoints need names
    return nullptr;

  if (waypoints != nullptr) {
    // Try to find waypoint by name
    auto from_database = waypoints->LookupName(name);

    // If waypoint by name found and closer than 10m to the original
    if (from_database != nullptr &&
        from_database->location.DistanceS(loc) <= 10)
      // Use this waypoint for the task
      return from_database;

    // Try finding the closest waypoint to the original one
    from_database = waypoints->GetNearest(loc, 10);

    // If closest waypoint found and closer than 10m to the original
    if (from_database != nullptr &&
        from_database->location.DistanceS(loc) <= 10)
      // Use this waypoint for the task
      return from_database;
  }

  // Create a new waypoint from the original one
  Waypoint *wp = new Waypoint(loc);
  wp->name = name;

  node.GetAttribute("id", wp->id);

  const char *comment = node.GetAttribute("comment");
  if (comment != nullptr)
    wp->comment = comment;

  if (node.GetAttribute("altitude", wp->elevation))
    wp->has_elevation = true;

  return WaypointPtr(wp);
}

static std::unique_ptr<ObservationZonePoint>
DeserialiseOZ(const Waypoint &wp, const ConstDataNode &node, bool is_turnpoint)
{
  const char *type = node.GetAttribute("type");
  if (type == nullptr)
    return nullptr;

  if (StringIsEqual(type, "Line")) {
    auto ls = std::make_unique<LineSectorZone>(wp.location);

    double length;
    if (node.GetAttribute("length", length) && length > 0)
      ls->SetLength(length);

    return ls;
  } else if (StringIsEqual(type, "Cylinder")) {
    auto ls = std::make_unique<CylinderZone>(wp.location);

    double radius;
    if (node.GetAttribute("radius", radius) && radius > 0)
      ls->SetRadius(radius);

    return ls;
  } else if (StringIsEqual(type, "MatCylinder")) {
    return CylinderZone::CreateMatCylinderZone(wp.location);
  } else if (StringIsEqual(type, "Sector")) {

    double radius, inner_radius;
    Angle start, end;
    std::unique_ptr<SectorZone> ls;

    if (node.GetAttribute("inner_radius", inner_radius)) {
      auto als = std::make_unique<AnnularSectorZone>(wp.location);
      als->SetInnerRadius(inner_radius);
      ls = std::move(als);
    } else
      ls = std::make_unique<SectorZone>(wp.location);

    if (node.GetAttribute("radius", radius) && radius > 0)
      ls->SetRadius(radius);
    if (node.GetAttribute("start_radial", start))
      ls->SetStartRadial(start);
    if (node.GetAttribute("end_radial", end))
      ls->SetEndRadial(end);

    return ls;
  } else if (StringIsEqual(type, "FAISector"))
    return SymmetricSectorZone::CreateFAISectorZone(wp.location, is_turnpoint);
  else if (StringIsEqual(type, "SymmetricQuadrant")) {
    double radius = 10000;
    node.GetAttribute("radius", radius);

    return std::make_unique<SymmetricSectorZone>(wp.location, radius);
  } else if (StringIsEqual(type, "Keyhole"))
    return KeyholeZone::CreateDAeCKeyholeZone(wp.location);
  else if (StringIsEqual(type, "CustomKeyhole")) {
    double radius = 10000, inner_radius = 500;
    Angle angle = Angle::QuarterCircle();

    node.GetAttribute("radius", radius);
    node.GetAttribute("inner_radius", inner_radius);
    node.GetAttribute("angle", angle);

    auto keyhole =
      KeyholeZone::CreateCustomKeyholeZone(wp.location, radius, angle);
    keyhole->SetInnerRadius(inner_radius);
    return keyhole;
  } else if (StringIsEqual(type, "BGAStartSector"))
    return KeyholeZone::CreateBGAStartSectorZone(wp.location);
  else if (StringIsEqual(type, "BGAFixedCourse"))
    return KeyholeZone::CreateBGAFixedCourseZone(wp.location);
  else if (StringIsEqual(type, "BGAEnhancedOption"))
    return KeyholeZone::CreateBGAEnhancedOptionZone(wp.location);

  return nullptr;
}

static void
DeserialiseTaskpoint(AbstractTaskFactory &fact, const ConstDataNode &node,
                     const Waypoints *waypoints)
{
  const char *type = node.GetAttribute("type");
  if (type == nullptr)
    return;

  auto wp_node = node.GetChildNamed("Waypoint");
  if (!wp_node)
    return;

  auto wp = DeserialiseWaypoint(*wp_node, waypoints);
  if (!wp)
    return;

  std::unique_ptr<ObservationZonePoint> oz;
  std::unique_ptr<OrderedTaskPoint> pt;

  if (auto oz_node = node.GetChildNamed("ObservationZone")) {
    bool is_turnpoint = StringIsEqual(type, "Turn") ||
      StringIsEqual(type, "Area");

    oz = DeserialiseOZ(*wp, *oz_node, is_turnpoint);
  }

  if (StringIsEqual(type, "Start")) {
    pt = oz != nullptr
      ? fact.CreateStart(std::move(oz), std::move(wp))
      : fact.CreateStart(std::move(wp));

  } else if (StringIsEqual(type, "OptionalStart")) {
    pt = oz != nullptr
      ? fact.CreateStart(std::move(oz), std::move(wp))
      : fact.CreateStart(std::move(wp));
    fact.AppendOptionalStart(*pt);

    // don't let generic code below add it
    pt.reset();

  } else if (StringIsEqual(type, "Turn")) {
    pt = oz != nullptr
      ? fact.CreateASTPoint(std::move(oz), std::move(wp))
      : fact.CreateIntermediate(std::move(wp));

  } else if (StringIsEqual(type, "Area")) {
    pt = oz != nullptr
      ? fact.CreateAATPoint(std::move(oz), std::move(wp))
      : fact.CreateIntermediate(std::move(wp));

  } else if (StringIsEqual(type, "Finish")) {
    pt = oz != nullptr
      ? fact.CreateFinish(std::move(oz), std::move(wp))
      : fact.CreateFinish(std::move(wp));
  } 

  if (!pt)
    return;

  if (pt->GetType() == TaskPointType::AST) {
    ASTPoint &ast = (ASTPoint &)*pt;
    bool score_exit = false;
    if (node.GetAttribute("score_exit", score_exit))
      ast.SetScoreExit(score_exit);
  }

  fact.Append(*pt, false);
}

static bool
GetHeightRef(const ConstDataNode &node, const char *nodename,
             AltitudeReference &value) noexcept
{
  const char *type = node.GetAttribute(nodename);
  if (type == nullptr) {
    return false;
  }
  if (StringIsEqual(type, "MSL")) {
    value = AltitudeReference::MSL;
  } else {
    value = AltitudeReference::AGL;
  }
  return true;
}

static void
Deserialise(OrderedTaskSettings &data, const ConstDataNode &node)
{
  node.GetAttribute("aat_min_time", data.aat_min_time);
  node.GetAttribute("start_requires_arm",
                    data.start_constraints.require_arm);
  node.GetAttribute("start_score_exit",
                    data.start_constraints.score_exit);
  node.GetAttribute("start_max_speed", data.start_constraints.max_speed);
  node.GetAttribute("start_max_height", data.start_constraints.max_height);
  GetHeightRef(node, "start_max_height_ref",
               data.start_constraints.max_height_ref);
  data.start_constraints.open_time_span =
    node.GetAttributeRoughTimeSpan("start_open_time",
                                   "start_close_time");
  node.GetAttribute("finish_min_height",
                    data.finish_constraints.min_height);

  GetHeightRef(node, "finish_min_height_ref",
               data.finish_constraints.min_height_ref);
  node.GetAttribute("fai_finish", data.finish_constraints.fai_finish);
  data.start_constraints.fai_finish = data.finish_constraints.fai_finish;
  node.GetAttribute("pev_start_wait_time",
                    data.start_constraints.pev_start_wait_time);
  node.GetAttribute("pev_start_window",
                    data.start_constraints.pev_start_window);

}

static TaskFactoryType
GetTaskFactoryType(const ConstDataNode &node)
{
  const char *type = node.GetAttribute("type");
  if (type == nullptr)
    return TaskFactoryType::FAI_GENERAL;

  if (StringIsEqual(type, "FAIGeneral"))
    return TaskFactoryType::FAI_GENERAL;
  else if (StringIsEqual(type, "FAITriangle"))
    return TaskFactoryType::FAI_TRIANGLE;
  else if (StringIsEqual(type, "FAIOR"))
    return TaskFactoryType::FAI_OR;
  else if (StringIsEqual(type, "FAIGoal"))
    return TaskFactoryType::FAI_GOAL;
  else if (StringIsEqual(type, "RT"))
    return TaskFactoryType::RACING;
  else if (StringIsEqual(type, "AAT"))
    return TaskFactoryType::AAT;
  else if (StringIsEqual(type, "MAT"))
    return TaskFactoryType::MAT;
  else if (StringIsEqual(type, "Mixed"))
    return TaskFactoryType::MIXED;
  else if (StringIsEqual(type, "Touring"))
    return TaskFactoryType::TOURING;

  return TaskFactoryType::FAI_GENERAL;
}

void
LoadTask(OrderedTask &task, const ConstDataNode &node,
         const Waypoints *waypoints)
{
  task.Clear();
  task.SetFactory(GetTaskFactoryType(node));
  task.Reset();

  OrderedTaskSettings beh = task.GetOrderedTaskSettings();
  Deserialise(beh, node);
  task.SetOrderedTaskSettings(beh);

  auto &fact = task.GetFactory();

  const auto children = node.ListChildrenNamed("Point");
  for (const auto &i : children) {
    DeserialiseTaskpoint(fact, *i, waypoints);
  }
}
