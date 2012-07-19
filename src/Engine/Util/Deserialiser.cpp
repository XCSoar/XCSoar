/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Task/OrderedTaskBehaviour.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/BGAFixedCourseZone.hpp"
#include "Task/ObservationZones/BGAEnhancedOptionZone.hpp"
#include "Task/ObservationZones/BGAStartSectorZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "DataNode.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#include <assert.h>

void
Deserialiser::deserialise_point(OrderedTask& data)
{
  tstring type;
  if (!m_node.GetAttribute(_T("type"), type)) {
    assert(1);
    return;
  }

  DataNode *wp_node = m_node.GetChildNamed(_T("Waypoint"));
  if (wp_node == NULL)
    return;

  Deserialiser wser(*wp_node, waypoints);
  Waypoint *wp = wser.deserialise_waypoint();
  if (wp == NULL) {
    delete wp_node;
    return;
  }

  DataNode *oz_node = m_node.GetChildNamed(_T("ObservationZone"));

  AbstractTaskFactory& fact = data.GetFactory();

  ObservationZonePoint* oz = NULL;
  OrderedTaskPoint *pt = NULL;

  if (oz_node != NULL) {
    bool is_turnpoint = StringIsEqual(type.c_str(), _T("Turn")) ||
                        StringIsEqual(type.c_str(), _T("Area"));

    Deserialiser oser(*oz_node, waypoints);
    oz = oser.deserialise_oz(*wp, is_turnpoint);
  }

  if (StringIsEqual(type.c_str(), _T("Start"))) {
    pt = (oz != NULL) ? fact.CreateStart(oz, *wp) : fact.CreateStart(*wp);

  } else if (StringIsEqual(type.c_str(), _T("OptionalStart"))) {
    pt = (oz != NULL) ? fact.CreateStart(oz, *wp) : fact.CreateStart(*wp);
    fact.AppendOptionalStart(*pt);
    delete pt; // don't let generic code below add it
    pt = NULL;

  } else if (StringIsEqual(type.c_str(), _T("Turn"))) {
    pt = (oz != NULL) ? fact.CreateASTPoint(oz, *wp)
                      : fact.CreateIntermediate(*wp);

  } else if (StringIsEqual(type.c_str(), _T("Area"))) {
    pt = (oz != NULL) ? fact.CreateAATPoint(oz, *wp)
                      : fact.CreateIntermediate(*wp);

  } else if (StringIsEqual(type.c_str(), _T("Finish"))) {
    pt = (oz != NULL) ? fact.CreateFinish(oz, *wp) : fact.CreateFinish(*wp);
  } 

  if (pt != NULL) {
    fact.Append(*pt, false);
    delete pt;
  }

  delete wp;
  delete wp_node;
  delete oz_node;
}

ObservationZonePoint*
Deserialiser::deserialise_oz(const Waypoint& wp, const bool is_turnpoint)
{
  tstring type;
  if (!m_node.GetAttribute(_T("type"), type)) {
    assert(1);
    return NULL;
  }

  if (StringIsEqual(type.c_str(), _T("Line"))) {
    LineSectorZone *ls = new LineSectorZone(wp.location);

    fixed length;
    if (m_node.GetAttribute(_T("length"), length) && positive(length))
      ls->SetLength(length);

    return ls;
  } else if (StringIsEqual(type.c_str(), _T("Cylinder"))) {
    CylinderZone *ls = new CylinderZone(wp.location);

    fixed radius;
    if (m_node.GetAttribute(_T("radius"), radius) && positive(radius))
      ls->SetRadius(radius);

    return ls;
  } else if (StringIsEqual(type.c_str(), _T("Sector"))) {

    fixed radius, inner_radius;
    Angle start, end;
    SectorZone *ls;

    if (m_node.GetAttribute(_T("inner_radius"), inner_radius)) {
      AnnularSectorZone *als = new AnnularSectorZone(wp.location);
      als->SetInnerRadius(inner_radius);
      ls = als;
    } else
      ls = new SectorZone(wp.location);

    if (m_node.GetAttribute(_T("radius"), radius) && positive(radius))
      ls->SetRadius(radius);
    if (m_node.GetAttribute(_T("start_radial"), start))
      ls->SetStartRadial(start);
    if (m_node.GetAttribute(_T("end_radial"), end))
      ls->SetEndRadial(end);

    return ls;
  } else if (StringIsEqual(type.c_str(), _T("FAISector")))
    return new FAISectorZone(wp.location, is_turnpoint);
  else if (StringIsEqual(type.c_str(), _T("Keyhole")))
    return new KeyholeZone(wp.location);
  else if (StringIsEqual(type.c_str(), _T("BGAStartSector")))
    return new BGAStartSectorZone(wp.location);
  else if (StringIsEqual(type.c_str(), _T("BGAFixedCourse")))
    return new BGAFixedCourseZone(wp.location);
  else if (StringIsEqual(type.c_str(), _T("BGAEnhancedOption")))
    return new BGAEnhancedOptionZone(wp.location);

  assert(1);
  return NULL;
}

void 
Deserialiser::deserialise(GeoPoint& data)
{
  m_node.GetAttribute(_T("longitude"), data.longitude);
  m_node.GetAttribute(_T("latitude"), data.latitude);
}

Waypoint*
Deserialiser::deserialise_waypoint()
{
  DataNode *loc_node = m_node.GetChildNamed(_T("Location"));
  if (!loc_node)
    return NULL;

  GeoPoint loc;
  Deserialiser lser(*loc_node, waypoints);
  lser.deserialise(loc);
  delete loc_node;

  tstring name;
  if (!m_node.GetAttribute(_T("name"), name))
    // Turnpoints need names
    return NULL;

  if (waypoints != NULL) {
    // Try to find waypoint by name
    const Waypoint *from_database = waypoints->LookupName(name);

    // If waypoint by name found and closer than 10m to the original
    if (from_database != NULL &&
        from_database->location.Distance(loc) <= fixed_ten)
      // Use this waypoint for the task
      return new Waypoint(*from_database);

    // Try finding the closest waypoint to the original one
    from_database = waypoints->GetNearest(loc, fixed_ten);

    // If closest waypoint found and closer than 10m to the original
    if (from_database != NULL &&
        from_database->location.Distance(loc) <= fixed_ten)
      // Use this waypoint for the task
      return new Waypoint(*from_database);
  }

  // Create a new waypoint from the original one
  Waypoint *wp = new Waypoint(loc);
  wp->name = name;
  m_node.GetAttribute(_T("id"), wp->id);
  m_node.GetAttribute(_T("comment"), wp->comment);
  m_node.GetAttribute(_T("altitude"), wp->elevation);

  return wp;
}

void 
Deserialiser::deserialise(OrderedTaskBehaviour& data)
{
  m_node.GetAttribute(_T("task_scored"), data.task_scored);
  m_node.GetAttribute(_T("aat_min_time"), data.aat_min_time);
  m_node.GetAttribute(_T("start_max_speed"), data.start_max_speed);
  m_node.GetAttribute(_T("start_max_height"), data.start_max_height);
  data.start_max_height_ref = height_ref(_T("start_max_height_ref"));
  m_node.GetAttribute(_T("finish_min_height"), data.finish_min_height);
  data.finish_min_height_ref = height_ref(_T("finish_min_height_ref"));
  m_node.GetAttribute(_T("fai_finish"), data.fai_finish);
  m_node.GetAttribute(_T("min_points"), data.min_points);
  m_node.GetAttribute(_T("max_points"), data.max_points);
  m_node.GetAttribute(_T("homogeneous_tps"), data.homogeneous_tps);
  m_node.GetAttribute(_T("is_closed"), data.is_closed);
}

void 
Deserialiser::deserialise(OrderedTask &task)
{
  task.Clear();
  task.SetFactory(task_factory_type());
  task.Reset();

  OrderedTaskBehaviour beh = task.get_ordered_task_behaviour();
  deserialise(beh);
  task.set_ordered_task_behaviour(beh);

  const DataNode::List children = m_node.ListChildrenNamed(_T("Point"));
  for (auto i = children.begin(), end = children.end(); i != end; ++i) {
    DataNode *point_node = *i;
    Deserialiser pser(*point_node, waypoints);
    pser.deserialise_point(task);
    delete point_node;
  }
}

HeightReferenceType
Deserialiser::height_ref(const TCHAR *nodename) const
{
  tstring type;
  if (m_node.GetAttribute(nodename, type) &&
      StringIsEqual(type.c_str(), _T("MSL")))
    return HeightReferenceType::MSL;

  return HeightReferenceType::AGL;
}

TaskFactoryType
Deserialiser::task_factory_type() const
{
  tstring type;
  if (!m_node.GetAttribute(_T("type"),type)) {
    assert(1);
    return TaskFactoryType::FAI_GENERAL;
  }

  if (StringIsEqual(type.c_str(), _T("FAIGeneral")))
    return TaskFactoryType::FAI_GENERAL;
  else if (StringIsEqual(type.c_str(), _T("FAITriangle")))
    return TaskFactoryType::FAI_TRIANGLE;
  else if (StringIsEqual(type.c_str(), _T("FAIOR")))
    return TaskFactoryType::FAI_OR;
  else if (StringIsEqual(type.c_str(), _T("FAIGoal")))
    return TaskFactoryType::FAI_GOAL;
  else if (StringIsEqual(type.c_str(), _T("RT")))
    return TaskFactoryType::RACING;
  else if (StringIsEqual(type.c_str(), _T("AAT")))
    return TaskFactoryType::AAT;
  else if (StringIsEqual(type.c_str(), _T("Mixed")))
    return TaskFactoryType::MIXED;
  else if (StringIsEqual(type.c_str(), _T("Touring")))
    return TaskFactoryType::TOURING;

  assert(1);
  return TaskFactoryType::FAI_GENERAL;
}
