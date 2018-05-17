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
  node.GetAttribute(_T("longitude"), data.longitude);
  node.GetAttribute(_T("latitude"), data.latitude);
}

static WaypointPtr
DeserialiseWaypoint(const ConstDataNode &node, const Waypoints *waypoints)
{
  std::unique_ptr<ConstDataNode> loc_node(node.GetChildNamed(_T("Location")));
  if (!loc_node)
    return nullptr;

  GeoPoint loc;
  Deserialise(loc, *loc_node);

  const TCHAR *name = node.GetAttribute(_T("name"));
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

  node.GetAttribute(_T("id"), wp->id);

  const TCHAR *comment = node.GetAttribute(_T("comment"));
  if (comment != nullptr)
    wp->comment.assign(comment);

  node.GetAttribute(_T("altitude"), wp->elevation);

  return WaypointPtr(wp);
}

static ObservationZonePoint *
DeserialiseOZ(const Waypoint &wp, const ConstDataNode &node, bool is_turnpoint)
{
  const TCHAR *type = node.GetAttribute(_T("type"));
  if (type == nullptr)
    return nullptr;

  if (StringIsEqual(type, _T("Line"))) {
    LineSectorZone *ls = new LineSectorZone(wp.location);

    double length;
    if (node.GetAttribute(_T("length"), length) && length > 0)
      ls->SetLength(length);

    return ls;
  } else if (StringIsEqual(type, _T("Cylinder"))) {
    CylinderZone *ls = new CylinderZone(wp.location);

    double radius;
    if (node.GetAttribute(_T("radius"), radius) && radius > 0)
      ls->SetRadius(radius);

    return ls;
  } else if (StringIsEqual(type, _T("MatCylinder"))) {
    return CylinderZone::CreateMatCylinderZone(wp.location);
  } else if (StringIsEqual(type, _T("Sector"))) {

    double radius, inner_radius;
    Angle start, end;
    SectorZone *ls;

    if (node.GetAttribute(_T("inner_radius"), inner_radius)) {
      AnnularSectorZone *als = new AnnularSectorZone(wp.location);
      als->SetInnerRadius(inner_radius);
      ls = als;
    } else
      ls = new SectorZone(wp.location);

    if (node.GetAttribute(_T("radius"), radius) && radius > 0)
      ls->SetRadius(radius);
    if (node.GetAttribute(_T("start_radial"), start))
      ls->SetStartRadial(start);
    if (node.GetAttribute(_T("end_radial"), end))
      ls->SetEndRadial(end);

    return ls;
  } else if (StringIsEqual(type, _T("FAISector")))
    return SymmetricSectorZone::CreateFAISectorZone(wp.location, is_turnpoint);
  else if (StringIsEqual(type, _T("SymmetricQuadrant"))) {
    double radius = 10000;
    node.GetAttribute(_T("radius"), radius);

    return new SymmetricSectorZone(wp.location, radius);
  } else if (StringIsEqual(type, _T("Keyhole")))
    return KeyholeZone::CreateDAeCKeyholeZone(wp.location);
  else if (StringIsEqual(type, _T("CustomKeyhole"))) {
    double radius = 10000, inner_radius = 500;
    Angle angle = Angle::QuarterCircle();

    node.GetAttribute(_T("radius"), radius);
    node.GetAttribute(_T("inner_radius"), inner_radius);
    node.GetAttribute(_T("angle"), angle);

    KeyholeZone *keyhole =
      KeyholeZone::CreateCustomKeyholeZone(wp.location, radius, angle);
    keyhole->SetInnerRadius(inner_radius);
    return keyhole;
  } else if (StringIsEqual(type, _T("BGAStartSector")))
    return KeyholeZone::CreateBGAStartSectorZone(wp.location);
  else if (StringIsEqual(type, _T("BGAFixedCourse")))
    return KeyholeZone::CreateBGAFixedCourseZone(wp.location);
  else if (StringIsEqual(type, _T("BGAEnhancedOption")))
    return KeyholeZone::CreateBGAEnhancedOptionZone(wp.location);

  return nullptr;
}

static void
DeserialiseTaskpoint(OrderedTask &data, const ConstDataNode &node,
                     const Waypoints *waypoints)
{
  const TCHAR *type = node.GetAttribute(_T("type"));
  if (type == nullptr)
    return;

  std::unique_ptr<ConstDataNode> wp_node(node.GetChildNamed(_T("Waypoint")));
  if (!wp_node)
    return;

  auto wp = DeserialiseWaypoint(*wp_node, waypoints);
  if (!wp)
    return;

  std::unique_ptr<ConstDataNode> oz_node(node.GetChildNamed(_T("ObservationZone")));

  AbstractTaskFactory &fact = data.GetFactory();

  ObservationZonePoint* oz = nullptr;
  std::unique_ptr<OrderedTaskPoint> pt;

  if (oz_node) {
    bool is_turnpoint = StringIsEqual(type, _T("Turn")) ||
      StringIsEqual(type, _T("Area"));

    oz = DeserialiseOZ(*wp, *oz_node, is_turnpoint);
  }

  if (StringIsEqual(type, _T("Start"))) {
    pt.reset(oz != nullptr
             ? fact.CreateStart(oz, std::move(wp))
             : fact.CreateStart(std::move(wp)));

  } else if (StringIsEqual(type, _T("OptionalStart"))) {
    pt.reset(oz != nullptr
             ? fact.CreateStart(oz, std::move(wp))
             : fact.CreateStart(std::move(wp)));
    fact.AppendOptionalStart(*pt);

    // don't let generic code below add it
    pt.reset();

  } else if (StringIsEqual(type, _T("Turn"))) {
    pt.reset(oz != nullptr
             ? fact.CreateASTPoint(oz, std::move(wp))
             : fact.CreateIntermediate(std::move(wp)));

  } else if (StringIsEqual(type, _T("Area"))) {
    pt.reset(oz != nullptr
             ? fact.CreateAATPoint(oz, std::move(wp))
             : fact.CreateIntermediate(std::move(wp)));

  } else if (StringIsEqual(type, _T("Finish"))) {
    pt.reset(oz != nullptr
             ? fact.CreateFinish(oz, std::move(wp))
             : fact.CreateFinish(std::move(wp)));
  } 

  if (!pt)
    return;

  if (pt->GetType() == TaskPointType::AST) {
    ASTPoint &ast = (ASTPoint &)*pt;
    bool score_exit = false;
    if (node.GetAttribute(_T("score_exit"), score_exit))
      ast.SetScoreExit(score_exit);
  }

  fact.Append(*pt, false);
}

gcc_pure
static AltitudeReference
GetHeightRef(const ConstDataNode &node, const TCHAR *nodename)
{
  const TCHAR *type = node.GetAttribute(nodename);
  if (type != nullptr && StringIsEqual(type, _T("MSL")))
    return AltitudeReference::MSL;

  return AltitudeReference::AGL;
}

static void
Deserialise(OrderedTaskSettings &data, const ConstDataNode &node)
{
  node.GetAttribute(_T("aat_min_time"), data.aat_min_time);
  node.GetAttribute(_T("start_requires_arm"),
                    data.start_constraints.require_arm);
  node.GetAttribute(_T("start_max_speed"), data.start_constraints.max_speed);
  node.GetAttribute(_T("start_max_height"), data.start_constraints.max_height);
  data.start_constraints.max_height_ref =
    GetHeightRef(node, _T("start_max_height_ref"));
  data.start_constraints.open_time_span =
    node.GetAttributeRoughTimeSpan(_T("start_open_time"),
                                   _T("start_close_time"));
  node.GetAttribute(_T("finish_min_height"),
                    data.finish_constraints.min_height);
  data.finish_constraints.min_height_ref =
    GetHeightRef(node, _T("finish_min_height_ref"));
  node.GetAttribute(_T("fai_finish"), data.finish_constraints.fai_finish);
  data.start_constraints.fai_finish = data.finish_constraints.fai_finish;
}

static TaskFactoryType
GetTaskFactoryType(const ConstDataNode &node)
{
  const TCHAR *type = node.GetAttribute(_T("type"));
  if (type == nullptr)
    return TaskFactoryType::FAI_GENERAL;

  if (StringIsEqual(type, _T("FAIGeneral")))
    return TaskFactoryType::FAI_GENERAL;
  else if (StringIsEqual(type, _T("FAITriangle")))
    return TaskFactoryType::FAI_TRIANGLE;
  else if (StringIsEqual(type, _T("FAIOR")))
    return TaskFactoryType::FAI_OR;
  else if (StringIsEqual(type, _T("FAIGoal")))
    return TaskFactoryType::FAI_GOAL;
  else if (StringIsEqual(type, _T("RT")))
    return TaskFactoryType::RACING;
  else if (StringIsEqual(type, _T("AAT")))
    return TaskFactoryType::AAT;
  else if (StringIsEqual(type, _T("MAT")))
    return TaskFactoryType::MAT;
  else if (StringIsEqual(type, _T("Mixed")))
    return TaskFactoryType::MIXED;
  else if (StringIsEqual(type, _T("Touring")))
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

  const auto children = node.ListChildrenNamed(_T("Point"));
  for (const auto &i : children) {
    std::unique_ptr<ConstDataNode> point_node(i);
    DeserialiseTaskpoint(task, *point_node, waypoints);
  }
}
