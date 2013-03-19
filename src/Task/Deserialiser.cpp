/* Copyright_License {

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

#include "Deserialiser.hpp"
#include "Task/Ordered/OrderedTaskBehaviour.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Task/Ordered/Points/StartPoint.hpp"
#include "Task/Ordered/Points/FinishPoint.hpp"
#include "Task/Ordered/Points/AATPoint.hpp"
#include "Task/Ordered/Points/ASTPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/BGAFixedCourseZone.hpp"
#include "Task/ObservationZones/BGAEnhancedOptionZone.hpp"
#include "Task/ObservationZones/BGAStartSectorZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Task/ObservationZones/MatCylinderZone.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "XML/DataNode.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#include <assert.h>
#include <memory>

void
Deserialiser::DeserialiseTaskpoint(OrderedTask &data)
{
  const TCHAR *type = node.GetAttribute(_T("type"));
  if (type == nullptr)
    return;

  std::unique_ptr<DataNode> wp_node(node.GetChildNamed(_T("Waypoint")));
  if (!wp_node)
    return;

  Deserialiser wser(*wp_node, waypoints);
  std::unique_ptr<Waypoint> wp(wser.DeserialiseWaypoint());
  if (!wp)
    return;

  std::unique_ptr<DataNode> oz_node(node.GetChildNamed(_T("ObservationZone")));

  AbstractTaskFactory &fact = data.GetFactory();

  ObservationZonePoint* oz = nullptr;
  std::unique_ptr<OrderedTaskPoint> pt;

  if (oz_node) {
    bool is_turnpoint = StringIsEqual(type, _T("Turn")) ||
      StringIsEqual(type, _T("Area"));

    Deserialiser oser(*oz_node, waypoints);
    oz = oser.DeserialiseOZ(*wp, is_turnpoint);
  }

  if (StringIsEqual(type, _T("Start"))) {
    pt.reset(oz != nullptr
             ? fact.CreateStart(oz, *wp)
             : fact.CreateStart(*wp));

  } else if (StringIsEqual(type, _T("OptionalStart"))) {
    pt.reset(oz != nullptr
             ? fact.CreateStart(oz, *wp)
             : fact.CreateStart(*wp));
    fact.AppendOptionalStart(*pt);

    // don't let generic code below add it
    pt.reset();

  } else if (StringIsEqual(type, _T("Turn"))) {
    pt.reset(oz != nullptr
             ? fact.CreateASTPoint(oz, *wp)
             : fact.CreateIntermediate(*wp));

  } else if (StringIsEqual(type, _T("Area"))) {
    pt.reset(oz != nullptr
             ? fact.CreateAATPoint(oz, *wp)
             : fact.CreateIntermediate(*wp));

  } else if (StringIsEqual(type, _T("Finish"))) {
    pt.reset(oz != nullptr
             ? fact.CreateFinish(oz, *wp)
             : fact.CreateFinish(*wp));
  } 

  if (pt)
    fact.Append(*pt, false);
}

ObservationZonePoint*
Deserialiser::DeserialiseOZ(const Waypoint &wp, bool is_turnpoint)
{
  const TCHAR *type = node.GetAttribute(_T("type"));
  if (type == nullptr)
    return nullptr;

  if (StringIsEqual(type, _T("Line"))) {
    LineSectorZone *ls = new LineSectorZone(wp.location);

    fixed length;
    if (node.GetAttribute(_T("length"), length) && positive(length))
      ls->SetLength(length);

    return ls;
  } else if (StringIsEqual(type, _T("Cylinder"))) {
    CylinderZone *ls = new CylinderZone(wp.location);

    fixed radius;
    if (node.GetAttribute(_T("radius"), radius) && positive(radius))
      ls->SetRadius(radius);

    return ls;
  } else if (StringIsEqual(type, _T("MatCylinder"))) {
    MatCylinderZone *ls = new MatCylinderZone(wp.location);

    return ls;
  } else if (StringIsEqual(type, _T("Sector"))) {

    fixed radius, inner_radius;
    Angle start, end;
    SectorZone *ls;

    if (node.GetAttribute(_T("inner_radius"), inner_radius)) {
      AnnularSectorZone *als = new AnnularSectorZone(wp.location);
      als->SetInnerRadius(inner_radius);
      ls = als;
    } else
      ls = new SectorZone(wp.location);

    if (node.GetAttribute(_T("radius"), radius) && positive(radius))
      ls->SetRadius(radius);
    if (node.GetAttribute(_T("start_radial"), start))
      ls->SetStartRadial(start);
    if (node.GetAttribute(_T("end_radial"), end))
      ls->SetEndRadial(end);

    return ls;
  } else if (StringIsEqual(type, _T("FAISector")))
    return new FAISectorZone(wp.location, is_turnpoint);
  else if (StringIsEqual(type, _T("SymmetricQuadrant"))) {
    fixed radius = fixed(10000);
    node.GetAttribute(_T("radius"), radius);

    return new SymmetricSectorZone(wp.location, radius);
  } else if (StringIsEqual(type, _T("Keyhole")))
    return KeyholeZone::CreateDAeCKeyholeZone(wp.location);
  else if (StringIsEqual(type, _T("BGAStartSector")))
    return new BGAStartSectorZone(wp.location);
  else if (StringIsEqual(type, _T("BGAFixedCourse")))
    return new BGAFixedCourseZone(wp.location);
  else if (StringIsEqual(type, _T("BGAEnhancedOption")))
    return new BGAEnhancedOptionZone(wp.location);

  assert(1);
  return nullptr;
}

void 
Deserialiser::Deserialise(GeoPoint &data)
{
  node.GetAttribute(_T("longitude"), data.longitude);
  node.GetAttribute(_T("latitude"), data.latitude);
}

Waypoint*
Deserialiser::DeserialiseWaypoint()
{
  std::unique_ptr<DataNode> loc_node(node.GetChildNamed(_T("Location")));
  if (!loc_node)
    return nullptr;

  GeoPoint loc;
  Deserialiser lser(*loc_node, waypoints);
  lser.Deserialise(loc);

  const TCHAR *name = node.GetAttribute(_T("name"));
  if (name == nullptr)
    // Turnpoints need names
    return nullptr;

  if (waypoints != nullptr) {
    // Try to find waypoint by name
    const Waypoint *from_database = waypoints->LookupName(name);

    // If waypoint by name found and closer than 10m to the original
    if (from_database != nullptr &&
        from_database->location.Distance(loc) <= fixed(10))
      // Use this waypoint for the task
      return new Waypoint(*from_database);

    // Try finding the closest waypoint to the original one
    from_database = waypoints->GetNearest(loc, fixed(10));

    // If closest waypoint found and closer than 10m to the original
    if (from_database != nullptr &&
        from_database->location.Distance(loc) <= fixed(10))
      // Use this waypoint for the task
      return new Waypoint(*from_database);
  }

  // Create a new waypoint from the original one
  Waypoint *wp = new Waypoint(loc);
  wp->name = name;

  node.GetAttribute(_T("id"), wp->id);

  const TCHAR *comment = node.GetAttribute(_T("comment"));
  if (comment != nullptr)
    wp->comment.assign(comment);

  node.GetAttribute(_T("altitude"), wp->elevation);

  return wp;
}

void 
Deserialiser::Deserialise(OrderedTaskBehaviour &data)
{
  node.GetAttribute(_T("aat_min_time"), data.aat_min_time);
  node.GetAttribute(_T("start_max_speed"), data.start_constraints.max_speed);
  node.GetAttribute(_T("start_max_height"), data.start_constraints.max_height);
  data.start_constraints.max_height_ref =
    GetHeightRef(_T("start_max_height_ref"));
  data.start_constraints.open_time_span =
    node.GetAttributeRoughTimeSpan(_T("start_open_time"),
                                   _T("start_close_time"));
  node.GetAttribute(_T("finish_min_height"),
                    data.finish_constraints.min_height);
  data.finish_constraints.min_height_ref =
    GetHeightRef(_T("finish_min_height_ref"));
  node.GetAttribute(_T("fai_finish"), data.finish_constraints.fai_finish);
  data.start_constraints.fai_finish = data.finish_constraints.fai_finish;
}

void 
Deserialiser::Deserialise(OrderedTask &task)
{
  task.Clear();
  task.SetFactory(GetTaskFactoryType());
  task.Reset();

  OrderedTaskBehaviour beh = task.GetOrderedTaskBehaviour();
  Deserialise(beh);
  task.SetOrderedTaskBehaviour(beh);

  const DataNode::List children = node.ListChildrenNamed(_T("Point"));
  for (const auto &i : children) {
    std::unique_ptr<DataNode> point_node(i);
    Deserialiser pser(*point_node, waypoints);
    pser.DeserialiseTaskpoint(task);
  }
}

AltitudeReference
Deserialiser::GetHeightRef(const TCHAR *nodename) const
{
  const TCHAR *type = node.GetAttribute(nodename);
  if (type != nullptr && StringIsEqual(type, _T("MSL")))
    return AltitudeReference::MSL;

  return AltitudeReference::AGL;
}

TaskFactoryType
Deserialiser::GetTaskFactoryType() const
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

  assert(1);
  return TaskFactoryType::FAI_GENERAL;
}
