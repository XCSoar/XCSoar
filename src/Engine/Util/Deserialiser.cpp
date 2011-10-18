/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
  if (!m_node.get_attribute(_T("type"), type)) {
    assert(1);
    return;
  }

  DataNode* wp_node = m_node.get_child_by_name(_T("Waypoint"));
  if (wp_node == NULL)
    return;

  Deserialiser wser(*wp_node, waypoints);
  Waypoint *wp = wser.deserialise_waypoint();
  if (wp == NULL) {
    delete wp_node;
    return;
  }

  DataNode* oz_node = m_node.get_child_by_name(_T("ObservationZone"));
  if (oz_node == NULL) {
    delete wp_node;
    delete wp;
    return;
  }

  Deserialiser oser(*oz_node, waypoints);

  AbstractTaskFactory& fact = data.GetFactory();

  ObservationZonePoint* oz = NULL;
  OrderedTaskPoint *pt = NULL;

  if (_tcscmp(type.c_str(), _T("Start")) == 0) {
    if ((oz = oser.deserialise_oz(*wp, false)) != NULL)
      pt = fact.createStart(oz, *wp);
  } else if (_tcscmp(type.c_str(), _T("OptionalStart")) == 0) {
    if ((oz = oser.deserialise_oz(*wp, false)) != NULL) {
      pt = fact.createStart(oz, *wp);
      fact.append_optional_start(*pt);
      delete pt; // don't let generic code below add it
      pt = NULL;
    }
  } else if (_tcscmp(type.c_str(), _T("Turn")) == 0) {
    if ((oz = oser.deserialise_oz(*wp, true)) != NULL)
      pt = fact.createAST(oz, *wp);
  } else if (_tcscmp(type.c_str(), _T("Area")) == 0) {
    if ((oz = oser.deserialise_oz(*wp, true)) != NULL)
      pt = fact.createAAT(oz, *wp);
  } else if (_tcscmp(type.c_str(), _T("Finish")) == 0) {
    if ((oz = oser.deserialise_oz(*wp, false)) != NULL)
      pt = fact.createFinish(oz, *wp);
  } 

  if (pt != NULL) {
    fact.append(*pt, false);
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
  if (!m_node.get_attribute(_T("type"), type)) {
    assert(1);
    return NULL;
  }

  if (_tcscmp(type.c_str(), _T("Line")) == 0) {
    LineSectorZone *ls = new LineSectorZone(wp.location);

    fixed length;
    if (m_node.get_attribute(_T("length"), length))
      ls->setLength(length);

    return ls;
  } else if (_tcscmp(type.c_str(), _T("Cylinder")) == 0) {
    CylinderZone *ls = new CylinderZone(wp.location);

    fixed radius;
    if (m_node.get_attribute(_T("radius"), radius))
      ls->setRadius(radius);

    return ls;
  } else if (_tcscmp(type.c_str(), _T("Sector")) == 0) {

    fixed radius, inner_radius;
    Angle start, end;
    SectorZone *ls;

    if (m_node.get_attribute(_T("inner_radius"), inner_radius)) {
      AnnularSectorZone *als = new AnnularSectorZone(wp.location);
      als->setInnerRadius(inner_radius);
      ls = als;
    } else
      ls = new SectorZone(wp.location);

    if (m_node.get_attribute(_T("radius"), radius))
      ls->setRadius(radius);
    if (m_node.get_attribute(_T("start_radial"), start))
      ls->setStartRadial(start);
    if (m_node.get_attribute(_T("end_radial"), end))
      ls->setEndRadial(end);

    return ls;
  } else if (_tcscmp(type.c_str(), _T("FAISector")) == 0)
    return new FAISectorZone(wp.location, is_turnpoint);
  else if (_tcscmp(type.c_str(), _T("Keyhole")) == 0)
    return new KeyholeZone(wp.location);
  else if (_tcscmp(type.c_str(), _T("BGAStartSector")) == 0)
    return new BGAStartSectorZone(wp.location);
  else if (_tcscmp(type.c_str(), _T("BGAFixedCourse")) == 0)
    return new BGAFixedCourseZone(wp.location);
  else if (_tcscmp(type.c_str(), _T("BGAEnhancedOption")) == 0)
    return new BGAEnhancedOptionZone(wp.location);

  assert(1);
  return NULL;
}

void 
Deserialiser::deserialise(GeoPoint& data)
{
  m_node.get_attribute(_T("longitude"), data.longitude);
  m_node.get_attribute(_T("latitude"), data.latitude);
}

Waypoint*
Deserialiser::deserialise_waypoint()
{
  DataNode* loc_node = m_node.get_child_by_name(_T("Location"));
  if (!loc_node)
    return NULL;

  GeoPoint loc;
  Deserialiser lser(*loc_node, waypoints);
  lser.deserialise(loc);
  delete loc_node;

  tstring name;
  if (!m_node.get_attribute(_T("name"), name))
    // Turnpoints need names
    return NULL;

  if (waypoints != NULL) {
    // Try to find waypoint by name
    const Waypoint *from_database = waypoints->lookup_name(name);

    // If waypoint by name found and closer than 10m to the original
    if (from_database != NULL &&
        from_database->location.Distance(loc) <= fixed_ten)
      // Use this waypoint for the task
      return new Waypoint(*from_database);

    // Try finding the closest waypoint to the original one
    from_database = waypoints->get_nearest(loc, fixed_ten);

    // If closest waypoint found and closer than 10m to the original
    if (from_database != NULL &&
        from_database->location.Distance(loc) <= fixed_ten)
      // Use this waypoint for the task
      return new Waypoint(*from_database);
  }

  // Create a new waypoint from the original one
  Waypoint *wp = new Waypoint(loc);
  wp->name = name;
  m_node.get_attribute(_T("id"), wp->id);
  m_node.get_attribute(_T("comment"), wp->comment);
  m_node.get_attribute(_T("altitude"), wp->altitude);

  return wp;
}

void 
Deserialiser::deserialise(OrderedTaskBehaviour& data)
{
  m_node.get_attribute(_T("task_scored"), data.task_scored);
  m_node.get_attribute(_T("aat_min_time"), data.aat_min_time);
  m_node.get_attribute(_T("start_max_speed"), data.start_max_speed);
  m_node.get_attribute(_T("start_max_height"), data.start_max_height);
  data.start_max_height_ref = height_ref(_T("start_max_height_ref"));
  m_node.get_attribute(_T("finish_min_height"), data.finish_min_height);
  data.finish_min_height_ref = height_ref(_T("finish_min_height_ref"));
  m_node.get_attribute(_T("fai_finish"), data.fai_finish);
  m_node.get_attribute(_T("min_points"), data.min_points);
  m_node.get_attribute(_T("max_points"), data.max_points);
  m_node.get_attribute(_T("homogeneous_tps"), data.homogeneous_tps);
  m_node.get_attribute(_T("is_closed"), data.is_closed);
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
  for (DataNode::List::const_iterator i = children.begin(),
         end = children.end(); i != end; ++i) {
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
  if (!m_node.get_attribute(nodename, type)) {
    return hrAGL;
  }

  if (_tcscmp(type.c_str(), _T("AGL")) == 0)
    return hrAGL;

  return hrMSL;
}

TaskBehaviour::FactoryType
Deserialiser::task_factory_type() const
{
  tstring type;
  if (!m_node.get_attribute(_T("type"),type)) {
    assert(1);
    return TaskBehaviour::FACTORY_FAI_GENERAL;
  }

  if (_tcscmp(type.c_str(), _T("FAIGeneral")) == 0)
    return TaskBehaviour::FACTORY_FAI_GENERAL;
  else if (_tcscmp(type.c_str(), _T("FAITriangle")) == 0)
    return TaskBehaviour::FACTORY_FAI_TRIANGLE;
  else if (_tcscmp(type.c_str(), _T("FAIOR")) == 0)
    return TaskBehaviour::FACTORY_FAI_OR;
  else if (_tcscmp(type.c_str(), _T("FAIGoal")) == 0)
    return TaskBehaviour::FACTORY_FAI_GOAL;
  else if (_tcscmp(type.c_str(), _T("RT")) == 0)
    return TaskBehaviour::FACTORY_RT;
  else if (_tcscmp(type.c_str(), _T("AAT")) == 0)
    return TaskBehaviour::FACTORY_AAT;
  else if (_tcscmp(type.c_str(), _T("Mixed")) == 0)
    return TaskBehaviour::FACTORY_MIXED;
  else if (_tcscmp(type.c_str(), _T("Touring")) == 0)
    return TaskBehaviour::FACTORY_TOURING;

  assert(1);
  return TaskBehaviour::FACTORY_FAI_GENERAL;
}
