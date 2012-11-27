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

#include "Serialiser.hpp"
#include "Task/Ordered/OrderedTaskBehaviour.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Task/Ordered/Points/StartPoint.hpp"
#include "Task/Ordered/Points/FinishPoint.hpp"
#include "Task/Ordered/Points/AATPoint.hpp"
#include "Task/Ordered/Points/ASTPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "XML/DataNode.hpp"

#include "Compiler.h"
#include <assert.h>
#include <memory>

gcc_const
static const TCHAR *
GetName(TaskPoint::Type type, bool mode_optional_start)
{
  switch (type) {
  case TaskPoint::ROUTE:
  case TaskPoint::UNORDERED:
    assert(false);
    break;

  case TaskPoint::START:
    return mode_optional_start ? _T("OptionalStart") : _T("Start");

  case TaskPoint::AST:
    return _T("Turn");

  case TaskPoint::AAT:
    return _T("Area");

  case TaskPoint::FINISH:
    return _T("Finish");
  }

  return nullptr;
}

gcc_pure
static const TCHAR *
GetName(const OrderedTaskPoint &tp, bool mode_optional_start)
{
  return GetName(tp.GetType(), mode_optional_start);
}

void
Serialiser::Serialise(const OrderedTaskPoint &data, const TCHAR* name)
{
  // do nothing
  std::unique_ptr<DataNode> child(node.AppendChild(_T("Point")));
  child->SetAttribute(_T("type"), name);

  std::unique_ptr<DataNode> wchild(child->AppendChild(_T("Waypoint")));
  Serialiser wser(*wchild);
  wser.Serialise(data.GetWaypoint());

  std::unique_ptr<DataNode> ochild(child->AppendChild(_T("ObservationZone")));
  Serialiser oser(*ochild);
  oser.Serialise(data.GetObservationZone());
}

void
Serialiser::Serialise(const OrderedTaskPoint &tp)
{
  const TCHAR *name = GetName(tp, mode_optional_start);
  assert(name != nullptr);
  Serialise(tp, name);
}

void 
Serialiser::Serialise(const ObservationZonePoint &data)
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
Serialiser::Visit(gcc_unused const FAISectorZone &data)
{
  node.SetAttribute(_T("type"), _T("FAISector"));
}

void 
Serialiser::Visit(gcc_unused const KeyholeZone &data)
{
  node.SetAttribute(_T("type"), _T("Keyhole"));
}

void 
Serialiser::Visit(gcc_unused const BGAFixedCourseZone &data)
{
  node.SetAttribute(_T("type"), _T("BGAFixedCourse"));
}

void 
Serialiser::Visit(gcc_unused const BGAEnhancedOptionZone &data)
{
  node.SetAttribute(_T("type"), _T("BGAEnhancedOption"));
}

void 
Serialiser::Visit(gcc_unused const BGAStartSectorZone &data)
{
  node.SetAttribute(_T("type"), _T("BGAStartSector"));
}

void
Serialiser::Visit(const SectorZone &data)
{
  node.SetAttribute(_T("type"), _T("Sector"));
  node.SetAttribute(_T("radius"), data.GetRadius());
  node.SetAttribute(_T("start_radial"), data.GetStartRadial());
  node.SetAttribute(_T("end_radial"), data.GetEndRadial());
}

void
Serialiser::Visit(const AnnularSectorZone &data)
{
  Visit((const SectorZone&)data);
  node.SetAttribute(_T("inner_radius"), data.GetInnerRadius());
}

void 
Serialiser::Visit(const LineSectorZone &data)
{
  node.SetAttribute(_T("type"), _T("Line"));
  node.SetAttribute(_T("length"), data.GetLength());
}

void 
Serialiser::Visit(const CylinderZone &data)
{
  node.SetAttribute(_T("type"), _T("Cylinder"));
  node.SetAttribute(_T("radius"), data.GetRadius());
}

void 
Serialiser::Serialise(const GeoPoint &data)
{
  node.SetAttribute(_T("longitude"), data.longitude);
  node.SetAttribute(_T("latitude"), data.latitude);
}

void 
Serialiser::Serialise(const Waypoint &data)
{
  node.SetAttribute(_T("name"), data.name.c_str());
  node.SetAttribute(_T("id"), data.id);
  node.SetAttribute(_T("comment"), data.comment.c_str());
  node.SetAttribute(_T("altitude"), data.elevation);

  std::unique_ptr<DataNode> child(node.AppendChild(_T("Location")));
  Serialiser ser(*child);
  ser.Serialise(data.location);
}

void 
Serialiser::Serialise(const OrderedTaskBehaviour &data)
{
  node.SetAttribute(_T("aat_min_time"), data.aat_min_time);
  node.SetAttribute(_T("start_max_speed"), data.start_constraints.max_speed);
  node.SetAttribute(_T("start_max_height"), data.start_constraints.max_height);
  node.SetAttribute(_T("start_max_height_ref"),
                       GetHeightRef(data.start_constraints.max_height_ref));
  node.SetAttribute(_T("start_open_time"),
                    data.start_constraints.open_time_span.GetStart());
  node.SetAttribute(_T("start_close_time"),
                    data.start_constraints.open_time_span.GetEnd());
  node.SetAttribute(_T("finish_min_height"),
                    data.finish_constraints.min_height);
  node.SetAttribute(_T("finish_min_height_ref"),
                       GetHeightRef(data.finish_constraints.min_height_ref));
  node.SetAttribute(_T("fai_finish"), data.finish_constraints.fai_finish);
}

void 
Serialiser::Serialise(const OrderedTask &task)
{
  node.SetAttribute(_T("type"), GetTaskFactoryType(task.GetFactoryType()));
  Serialise(task.GetOrderedTaskBehaviour());
  mode_optional_start = false;

  for (const auto &tp : task.GetPoints())
    Serialise(tp);

  mode_optional_start = true;
  for (const auto &tp : task.GetOptionalStartPoints())
    Serialise(tp);
}

const TCHAR*
Serialiser::GetHeightRef(AltitudeReference height_ref) const
{
  switch(height_ref) {
  case AltitudeReference::AGL:
    return _T("AGL");
  case AltitudeReference::MSL:
    return _T("MSL");

  case AltitudeReference::STD:
  case AltitudeReference::NONE:
    /* not applicable here */
    break;
  }
  return NULL;
}

const TCHAR* 
Serialiser::GetTaskFactoryType(TaskFactoryType type) const
{
  switch(type) {
  case TaskFactoryType::FAI_GENERAL:
    return _T("FAIGeneral");
  case TaskFactoryType::FAI_TRIANGLE:
    return _T("FAITriangle");
  case TaskFactoryType::FAI_OR:
    return _T("FAIOR");
  case TaskFactoryType::FAI_GOAL:
    return _T("FAIGoal");
  case TaskFactoryType::RACING:
    return _T("RT");
  case TaskFactoryType::AAT:
    return _T("AAT");
  case TaskFactoryType::MIXED:
    return _T("Mixed");
  case TaskFactoryType::TOURING:
    return _T("Touring");
  }

  return NULL;
}
