// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Declaration.hpp"
#include "Logger/Settings.hpp"
#include "Plane/Plane.hpp"
#include "Task/ObservationZones/ObservationZonePoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"

[[gnu::pure]]
static Declaration::TurnPoint::Shape
get_shape(const OrderedTaskPoint &tp)
{
  const ObservationZonePoint &oz = tp.GetObservationZone();
  switch (oz.GetShape()) {
  case ObservationZone::Shape::LINE:
    return Declaration::TurnPoint::LINE;

  case ObservationZone::Shape::MAT_CYLINDER:
  case ObservationZone::Shape::CYLINDER:
    return Declaration::TurnPoint::CYLINDER;
  case ObservationZone::Shape::DAEC_KEYHOLE:
    return Declaration::TurnPoint::DAEC_KEYHOLE;

  default:
    return Declaration::TurnPoint::SECTOR;
  }
}

[[gnu::pure]]
static unsigned
get_radius(const OrderedTaskPoint &tp)
{
  const CylinderZone &oz = (const CylinderZone &)tp.GetObservationZone();
  return (unsigned)oz.GetRadius();
}

Declaration::TurnPoint::TurnPoint(const OrderedTaskPoint &tp)
  :waypoint(tp.GetWaypoint()),
   shape(get_shape(tp)), radius(get_radius(tp))
{
}

Declaration::Declaration(const LoggerSettings &logger_settings,
                         const Plane &plane,
                         const OrderedTask* task)
  :pilot_name(logger_settings.pilot_name),
   copilot_name(logger_settings.copilot_name),
   aircraft_type(plane.type),
   aircraft_registration(plane.registration),
   competition_id(plane.competition_id)
{
  if (task)
    for (unsigned i = 0; i < task->TaskSize(); i++)
      turnpoints.push_back(task->GetTaskPoint(i));
}
