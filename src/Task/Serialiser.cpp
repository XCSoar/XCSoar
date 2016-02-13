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

#include "Serialiser.hpp"
#include "Task/Ordered/Settings.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Task/Ordered/Points/ASTPoint.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Task/ObservationZones/SymmetricSectorZone.hpp"
#include "XML/DataNode.hpp"
#include "Compiler.h"

#include <memory>

#include <assert.h>
#include <tchar.h>

gcc_const
static const TCHAR *
GetName(TaskPointType type, bool mode_optional_start)
{
  switch (type) {
  case TaskPointType::UNORDERED:
    gcc_unreachable();

  case TaskPointType::START:
    return mode_optional_start ? _T("OptionalStart") : _T("Start");

  case TaskPointType::AST:
    return _T("Turn");

  case TaskPointType::AAT:
    return _T("Area");

  case TaskPointType::FINISH:
    return _T("Finish");
  }

  gcc_unreachable();
}

gcc_pure
static const TCHAR *
GetName(const OrderedTaskPoint &tp, bool mode_optional_start)
{
  return GetName(tp.GetType(), mode_optional_start);
}

static void
Serialise(WritableDataNode &node, const GeoPoint &data)
{
  node.SetAttribute(_T("longitude"), data.longitude);
  node.SetAttribute(_T("latitude"), data.latitude);
}

static void
Serialise(WritableDataNode &node, const Waypoint &data)
{
  node.SetAttribute(_T("name"), data.name.c_str());
  node.SetAttribute(_T("id"), data.id);
  node.SetAttribute(_T("comment"), data.comment.c_str());
  node.SetAttribute(_T("altitude"), data.elevation);

  std::unique_ptr<WritableDataNode> child(node.AppendChild(_T("Location")));
  Serialise(*child, data.location);
}

static void
Visit(WritableDataNode &node, const SectorZone &data)
{
  node.SetAttribute(_T("type"), _T("Sector"));
  node.SetAttribute(_T("radius"), data.GetRadius());
  node.SetAttribute(_T("start_radial"), data.GetStartRadial());
  node.SetAttribute(_T("end_radial"), data.GetEndRadial());
}

static void
Visit(WritableDataNode &node, const SymmetricSectorZone &data)
{
  node.SetAttribute(_T("type"), _T("SymmetricQuadrant"));
  node.SetAttribute(_T("radius"), data.GetRadius());
  node.SetAttribute(_T("angle"), data.GetSectorAngle());
}

static void
Visit(WritableDataNode &node, const AnnularSectorZone &data)
{
  Visit(node, (const SectorZone &)data);
  node.SetAttribute(_T("inner_radius"), data.GetInnerRadius());
}

static void
Visit(WritableDataNode &node, const LineSectorZone &data)
{
  node.SetAttribute(_T("type"), _T("Line"));
  node.SetAttribute(_T("length"), data.GetLength());
}

static void
Visit(WritableDataNode &node, const CylinderZone &data)
{
  node.SetAttribute(_T("type"), _T("Cylinder"));
  node.SetAttribute(_T("radius"), data.GetRadius());
}

static void
Serialise(WritableDataNode &node, const ObservationZonePoint &data)
{
  switch (data.GetShape()) {
  case ObservationZone::Shape::FAI_SECTOR:
    node.SetAttribute(_T("type"), _T("FAISector"));
    break;

  case ObservationZone::Shape::SECTOR:
    Visit(node, (const SectorZone &)data);
    break;

  case ObservationZone::Shape::LINE:
    Visit(node, (const LineSectorZone &)data);
    break;

  case ObservationZone::Shape::MAT_CYLINDER:
    node.SetAttribute(_T("type"), _T("MatCylinder"));
    break;

  case ObservationZone::Shape::CYLINDER:
    Visit(node, (const CylinderZone &)data);
    break;

  case ObservationZone::Shape::CUSTOM_KEYHOLE: {
    const KeyholeZone &keyhole = (const KeyholeZone &)data;
    node.SetAttribute(_T("type"), _T("CustomKeyhole"));
    node.SetAttribute(_T("inner_radius"), keyhole.GetInnerRadius());
    break;
  }

  case ObservationZone::Shape::DAEC_KEYHOLE:
    node.SetAttribute(_T("type"), _T("Keyhole"));
    break;

  case ObservationZone::Shape::BGAFIXEDCOURSE:
    node.SetAttribute(_T("type"), _T("BGAFixedCourse"));
    break;

  case ObservationZone::Shape::BGAENHANCEDOPTION:
    node.SetAttribute(_T("type"), _T("BGAEnhancedOption"));
    break;

  case ObservationZone::Shape::BGA_START:
    node.SetAttribute(_T("type"), _T("BGAStartSector"));
    break;

  case ObservationZone::Shape::ANNULAR_SECTOR:
    Visit(node, (const AnnularSectorZone &)data);
    break;

  case ObservationZone::Shape::SYMMETRIC_QUADRANT:
    Visit(node, (const SymmetricSectorZone &)data);
    break;
  }
} 

static void
Serialise(WritableDataNode &node, const OrderedTaskPoint &data,
          const TCHAR *name)
{
  // do nothing
  std::unique_ptr<WritableDataNode> child(node.AppendChild(_T("Point")));
  child->SetAttribute(_T("type"), name);

  std::unique_ptr<WritableDataNode> wchild(child->AppendChild(_T("Waypoint")));
  Serialise(*wchild, data.GetWaypoint());

  std::unique_ptr<WritableDataNode> ochild(child->AppendChild(_T("ObservationZone")));
  Serialise(*ochild, data.GetObservationZone());

  if (data.GetType() == TaskPointType::AST) {
    const ASTPoint &ast = (const ASTPoint &)data;
    if (ast.GetScoreExit())
      child->SetAttribute(_T("score_exit"), true);
  }
}

static void
Serialise(WritableDataNode &node, const OrderedTaskPoint &tp,
          bool mode_optional_start)
{
  const TCHAR *name = GetName(tp, mode_optional_start);
  assert(name != nullptr);
  Serialise(node, tp, name);
}

gcc_const
static const TCHAR *
GetHeightRef(AltitudeReference height_ref)
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
  return nullptr;
}

gcc_const
static const TCHAR *
GetTaskFactoryType(TaskFactoryType type)
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
  case TaskFactoryType::MAT:
    return _T("MAT");
  case TaskFactoryType::MIXED:
    return _T("Mixed");
  case TaskFactoryType::TOURING:
    return _T("Touring");
  case TaskFactoryType::COUNT:
    gcc_unreachable();
  }

  gcc_unreachable();
}

static void
Serialise(WritableDataNode &node, const OrderedTaskSettings &data)
{
  node.SetAttribute(_T("aat_min_time"), data.aat_min_time);
  node.SetAttribute(_T("start_requires_arm"),
                    data.start_constraints.require_arm);
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
SaveTask(WritableDataNode &node, const OrderedTask &task)
{
  node.SetAttribute(_T("type"), GetTaskFactoryType(task.GetFactoryType()));
  Serialise(node, task.GetOrderedTaskSettings());

  for (const auto &tp : task.GetPoints())
    Serialise(node, tp, false);

  for (const auto &tp : task.GetOptionalStartPoints())
    Serialise(node, tp, true);
}
