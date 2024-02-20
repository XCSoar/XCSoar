// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "util/Compiler.h"
#include "util/ConvertString.hpp"

#include <cassert>
#include <tchar.h>

[[gnu::const]]
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

[[gnu::pure]]
static const TCHAR *
GetName(const OrderedTaskPoint &tp, bool mode_optional_start)
{
  return GetName(tp.GetType(), mode_optional_start);
}

static void
Serialise(WritableDataNode &node, const GeoPoint &data)
{
  node.SetAttribute("longitude", data.longitude);
  node.SetAttribute("latitude", data.latitude);
}

static void
Serialise(WritableDataNode &node, const Waypoint &data)
{
  node.SetAttribute("name", WideToUTF8Converter(data.name.c_str()));
  node.SetAttribute("id", data.id);
  node.SetAttribute("comment", WideToUTF8Converter(data.comment.c_str()));
  if (data.has_elevation)
    node.SetAttribute("altitude", data.elevation);

  Serialise(*node.AppendChild("Location"), data.location);
}

static void
Visit(WritableDataNode &node, const SectorZone &data)
{
  node.SetAttribute("type", "Sector");
  node.SetAttribute("radius", data.GetRadius());
  node.SetAttribute("start_radial", data.GetStartRadial());
  node.SetAttribute("end_radial", data.GetEndRadial());
}

static void
Visit(WritableDataNode &node, const SymmetricSectorZone &data)
{
  node.SetAttribute("type", "SymmetricQuadrant");
  node.SetAttribute("radius", data.GetRadius());
  node.SetAttribute("angle", data.GetSectorAngle());
}

static void
Visit(WritableDataNode &node, const AnnularSectorZone &data)
{
  Visit(node, (const SectorZone &)data);
  node.SetAttribute("inner_radius", data.GetInnerRadius());
}

static void
Visit(WritableDataNode &node, const LineSectorZone &data)
{
  node.SetAttribute("type", "Line");
  node.SetAttribute("length", data.GetLength());
}

static void
Visit(WritableDataNode &node, const CylinderZone &data)
{
  node.SetAttribute("type", "Cylinder");
  node.SetAttribute("radius", data.GetRadius());
}

static void
Serialise(WritableDataNode &node, const ObservationZonePoint &data)
{
  switch (data.GetShape()) {
  case ObservationZone::Shape::FAI_SECTOR:
    node.SetAttribute("type", "FAISector");
    break;

  case ObservationZone::Shape::SECTOR:
    Visit(node, (const SectorZone &)data);
    break;

  case ObservationZone::Shape::LINE:
    Visit(node, (const LineSectorZone &)data);
    break;

  case ObservationZone::Shape::MAT_CYLINDER:
    node.SetAttribute("type", "MatCylinder");
    break;

  case ObservationZone::Shape::CYLINDER:
    Visit(node, (const CylinderZone &)data);
    break;

  case ObservationZone::Shape::CUSTOM_KEYHOLE: {
    const KeyholeZone &keyhole = (const KeyholeZone &)data;
    node.SetAttribute("type", "CustomKeyhole");
    node.SetAttribute("angle", keyhole.GetSectorAngle());
    node.SetAttribute("radius", keyhole.GetRadius());
    node.SetAttribute("inner_radius", keyhole.GetInnerRadius());
    break;
  }

  case ObservationZone::Shape::DAEC_KEYHOLE:
    node.SetAttribute("type", "Keyhole");
    break;

  case ObservationZone::Shape::BGAFIXEDCOURSE:
    node.SetAttribute("type", "BGAFixedCourse");
    break;

  case ObservationZone::Shape::BGAENHANCEDOPTION:
    node.SetAttribute("type", "BGAEnhancedOption");
    break;

  case ObservationZone::Shape::BGA_START:
    node.SetAttribute("type", "BGAStartSector");
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
  auto child = node.AppendChild("Point");
  child->SetAttribute("type", WideToUTF8Converter(name));

  Serialise(*child->AppendChild("Waypoint"), data.GetWaypoint());
  Serialise(*child->AppendChild("ObservationZone"),
                      data.GetObservationZone());

  if (data.GetType() == TaskPointType::AST) {
    const ASTPoint &ast = (const ASTPoint &)data;
    if (ast.GetScoreExit())
      child->SetAttribute("score_exit", true);
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

[[gnu::const]]
static const char *
GetHeightRef(AltitudeReference height_ref)
{
  switch(height_ref) {
  case AltitudeReference::AGL:
    return ("AGL");
  case AltitudeReference::MSL:
    return ("MSL");

  case AltitudeReference::STD:
    /* not applicable here */
    break;
  }
  return nullptr;
}

[[gnu::const]]
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
  node.SetAttribute("aat_min_time", data.aat_min_time);
  node.SetAttribute("start_requires_arm",
                    data.start_constraints.require_arm);
  node.SetAttribute("start_score_exit",
                    data.start_constraints.score_exit);
  node.SetAttribute("start_max_speed", data.start_constraints.max_speed);
  node.SetAttribute("start_max_height", data.start_constraints.max_height);
  node.SetAttribute("start_max_height_ref",
                    GetHeightRef(data.start_constraints.max_height_ref));
  node.SetAttribute("start_open_time",
                    data.start_constraints.open_time_span.GetStart());
  node.SetAttribute("start_close_time",
                    data.start_constraints.open_time_span.GetEnd());
  node.SetAttribute("finish_min_height",
                    data.finish_constraints.min_height);
  node.SetAttribute("finish_min_height_ref",
                    GetHeightRef(data.finish_constraints.min_height_ref));
  node.SetAttribute("fai_finish", data.finish_constraints.fai_finish);
  node.SetAttribute("pev_start_wait_time",
                    data.start_constraints.pev_start_wait_time);
  node.SetAttribute("pev_start_window",
                    data.start_constraints.pev_start_window);
}

void
SaveTask(WritableDataNode &node, const OrderedTask &task)
{
  node.SetAttribute("type", WideToUTF8Converter(GetTaskFactoryType(task.GetFactoryType())));
  Serialise(node, task.GetOrderedTaskSettings());

  for (const auto &tp : task.GetPoints())
    Serialise(node, tp, false);

  for (const auto &tp : task.GetOptionalStartPoints())
    Serialise(node, tp, true);
}
