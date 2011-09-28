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

#include "Serialiser.hpp"
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

#include "Compiler.h"
#include <assert.h>

void
Serialiser::Visit(const StartPoint& data)
{
  DataNode* child =
    serialise(data, mode_optional_start ? _T("OptionalStart"): _T("Start"));
  delete child;
}

void
Serialiser::Visit(const ASTPoint& data)
{
  DataNode* child = serialise(data, _T("Turn"));
  delete child;
}

void
Serialiser::Visit(const AATPoint& data)
{
  DataNode* child = serialise(data, _T("Area"));
  delete child;
}

void
Serialiser::Visit(const FinishPoint& data)
{
  DataNode* child = serialise(data, _T("Finish"));
  delete child;
}

void
Serialiser::Visit(gcc_unused const UnorderedTaskPoint& data)
{
}

DataNode*
Serialiser::serialise(const OrderedTaskPoint& data, const TCHAR* name)
{
  // do nothing
  DataNode* child = m_node.add_child(_T("Point"));
  child->set_attribute(_T("type"), name);

  DataNode* wchild = child->add_child(_T("Waypoint"));
  Serialiser wser(*wchild, waypoints);
  wser.serialise(data.GetWaypoint());
  delete wchild;

  DataNode* ochild = child->add_child(_T("ObservationZone"));
  Serialiser oser(*ochild, waypoints);
  oser.serialise(*data.get_oz());
  delete ochild;

  return child;
}

void 
Serialiser::serialise(const ObservationZonePoint& data) 
{
  switch (data.shape) {
  case ObservationZonePoint::FAI_SECTOR:
    Visit((const FAISectorZone &)data);
    break;

  case ObservationZonePoint::SECTOR:
    Visit((const SectorZone &)data);
    break;

  case ObservationZonePoint::LINE:
    Visit((const LineSectorZone &)data);
    break;

  case ObservationZonePoint::CYLINDER:
    Visit((const CylinderZone &)data);
    break;

  case ObservationZonePoint::KEYHOLE:
    Visit((const KeyholeZone &)data);
    break;

  case ObservationZonePoint::BGAFIXEDCOURSE:
    Visit((const BGAFixedCourseZone &)data);
    break;

  case ObservationZonePoint::BGAENHANCEDOPTION:
    Visit((const BGAEnhancedOptionZone &)data);
    break;

  case ObservationZonePoint::BGA_START:
    Visit((const BGAStartSectorZone &)data);
    break;

  case ObservationZonePoint::ANNULAR_SECTOR:
    Visit((const AnnularSectorZone &)data);
    break;
  }
} 

void 
Serialiser::Visit(gcc_unused const FAISectorZone& data)
{
  m_node.set_attribute(_T("type"), _T("FAISector"));
}

void 
Serialiser::Visit(gcc_unused const KeyholeZone& data)
{
  m_node.set_attribute(_T("type"), _T("Keyhole"));
}

void 
Serialiser::Visit(gcc_unused const BGAFixedCourseZone& data)
{
  m_node.set_attribute(_T("type"), _T("BGAFixedCourse"));
}

void 
Serialiser::Visit(gcc_unused const BGAEnhancedOptionZone& data)
{
  m_node.set_attribute(_T("type"), _T("BGAEnhancedOption"));
}

void 
Serialiser::Visit(gcc_unused const BGAStartSectorZone& data)
{
  m_node.set_attribute(_T("type"), _T("BGAStartSector"));
}

void
Serialiser::Visit(const SectorZone& data)
{
  m_node.set_attribute(_T("type"), _T("Sector"));
  m_node.set_attribute(_T("radius"), data.getRadius());
  m_node.set_attribute(_T("start_radial"), data.getStartRadial());
  m_node.set_attribute(_T("end_radial"), data.getEndRadial());
}

void
Serialiser::Visit(const AnnularSectorZone& data)
{
  Visit((const SectorZone&)data);
  m_node.set_attribute(_T("inner_radius"), data.getInnerRadius());
}

void 
Serialiser::Visit(const LineSectorZone& data)
{
  m_node.set_attribute(_T("type"), _T("Line"));
  m_node.set_attribute(_T("length"), data.getLength());
}

void 
Serialiser::Visit(const CylinderZone& data)
{
  m_node.set_attribute(_T("type"), _T("Cylinder"));
  m_node.set_attribute(_T("radius"), data.getRadius());
}

void 
Serialiser::serialise(const GeoPoint& data)
{
  m_node.set_attribute(_T("longitude"), data.longitude);
  m_node.set_attribute(_T("latitude"), data.latitude);
}

void 
Serialiser::serialise(const Waypoint& data)
{
  m_node.set_attribute(_T("name"), data.name);
  m_node.set_attribute(_T("id"), data.id);
  m_node.set_attribute(_T("comment"), data.comment);
  m_node.set_attribute(_T("altitude"), data.altitude);

  DataNode* child = m_node.add_child(_T("Location"));
  Serialiser ser(*child, waypoints);
  ser.serialise(data.location);
  delete child;
}

void 
Serialiser::serialise(const OrderedTaskBehaviour& data)
{
  m_node.set_attribute(_T("task_scored"), data.task_scored);
  m_node.set_attribute(_T("aat_min_time"), data.aat_min_time);
  m_node.set_attribute(_T("start_max_speed"), data.start_max_speed);
  m_node.set_attribute(_T("start_max_height"), data.start_max_height);
  m_node.set_attribute(_T("start_max_height_ref"),
                       height_ref(data.start_max_height_ref));
  m_node.set_attribute(_T("finish_min_height"), data.finish_min_height);
  m_node.set_attribute(_T("finish_min_height_ref"),
                       height_ref(data.finish_min_height_ref));
  m_node.set_attribute(_T("fai_finish"), data.fai_finish);
  m_node.set_attribute(_T("min_points"), data.min_points);
  m_node.set_attribute(_T("max_points"), data.max_points);
  m_node.set_attribute(_T("homogeneous_tps"), data.homogeneous_tps);
  m_node.set_attribute(_T("is_closed"), data.is_closed);
}

void 
Serialiser::serialise(const OrderedTask &task)
{
  m_node.set_attribute(_T("type"), task_factory_type(task.get_factory_type()));
  serialise(task.get_ordered_task_behaviour());
  mode_optional_start = false;
  task.tp_CAccept(*this);
  mode_optional_start = true;
  task.sp_CAccept(*this);
}

const TCHAR*
Serialiser::height_ref(HeightReferenceType the_height_ref) const
{
  switch(the_height_ref) {
  case hrAGL:
    return _T("AGL");
  case hrMSL:
    return _T("MSL");
  }
  return NULL;
}

const TCHAR* 
Serialiser::task_factory_type(TaskBehaviour::Factory_t the_type) const
{
  switch(the_type) {
  case TaskBehaviour::FACTORY_FAI_GENERAL:
    return _T("FAIGeneral");
  case TaskBehaviour::FACTORY_FAI_TRIANGLE:
    return _T("FAITriangle");
  case TaskBehaviour::FACTORY_FAI_OR:
    return _T("FAIOR");
  case TaskBehaviour::FACTORY_FAI_GOAL:
    return _T("FAIGoal");
  case TaskBehaviour::FACTORY_RT:
    return _T("RT");
  case TaskBehaviour::FACTORY_AAT:
    return _T("AAT");
  case TaskBehaviour::FACTORY_MIXED:
    return _T("Mixed");
  case TaskBehaviour::FACTORY_TOURING:
    return _T("Touring");
  }

  return NULL;
}
