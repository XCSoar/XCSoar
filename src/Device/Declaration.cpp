// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Declaration.hpp"
#include "Logger/Settings.hpp"
#include "Plane/Plane.hpp"
#include "Task/ObservationZones/ObservationZonePoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Engine/Task/ObservationZones/SymmetricSectorZone.hpp"
#include "Engine/Task/ObservationZones/KeyholeZone.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/Factory/TaskFactoryType.hpp"

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
  case ObservationZone::Shape::CUSTOM_KEYHOLE:
  case ObservationZone::Shape::BGAFIXEDCOURSE:
  case ObservationZone::Shape::BGAENHANCEDOPTION:
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

/**
 * Extract the half-angle of the observation zone sector.
 * Returns 180 degrees for cylinders (full circle).
 */
[[gnu::pure]]
static Angle
get_sector_angle(const OrderedTaskPoint &tp)
{
  const ObservationZonePoint &oz = tp.GetObservationZone();
  switch (oz.GetShape()) {
  case ObservationZone::Shape::LINE:
    return Angle::QuarterCircle();

  case ObservationZone::Shape::MAT_CYLINDER:
  case ObservationZone::Shape::CYLINDER:
    return Angle::HalfCircle();

  case ObservationZone::Shape::FAI_SECTOR:
  case ObservationZone::Shape::SYMMETRIC_QUADRANT:
  case ObservationZone::Shape::DAEC_KEYHOLE:
  case ObservationZone::Shape::CUSTOM_KEYHOLE:
  case ObservationZone::Shape::BGAFIXEDCOURSE:
  case ObservationZone::Shape::BGAENHANCEDOPTION:
  case ObservationZone::Shape::BGA_START:
    return static_cast<const SymmetricSectorZone &>(oz).GetSectorAngle();

  case ObservationZone::Shape::SECTOR:
  case ObservationZone::Shape::ANNULAR_SECTOR: {
    /* For asymmetric sectors, compute the half-angle from the
       start/end radials */
    const auto &sz = static_cast<const SectorZone &>(oz);
    return (sz.GetEndRadial() - sz.GetStartRadial()).AsBearing() / 2;
  }
  }

  return Angle::HalfCircle();
}

/**
 * Extract the inner radius for keyhole-type observation zones.
 * Returns 0 for non-keyhole zones.
 */
[[gnu::pure]]
static unsigned
get_inner_radius(const OrderedTaskPoint &tp)
{
  const ObservationZonePoint &oz = tp.GetObservationZone();
  switch (oz.GetShape()) {
  case ObservationZone::Shape::DAEC_KEYHOLE:
  case ObservationZone::Shape::CUSTOM_KEYHOLE:
  case ObservationZone::Shape::BGAFIXEDCOURSE:
  case ObservationZone::Shape::BGAENHANCEDOPTION:
    return (unsigned)static_cast<const KeyholeZone &>(oz).GetInnerRadius();

  default:
    return 0;
  }
}

Declaration::TurnPoint::TurnPoint(const OrderedTaskPoint &tp)
  :waypoint(tp.GetWaypoint()),
   shape(get_shape(tp)), radius(get_radius(tp)),
   sector_angle(get_sector_angle(tp)),
   inner_radius(get_inner_radius(tp)),
   is_aat(tp.GetType() == TaskPointType::AAT)
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
  if (task) {
    const auto ft = task->GetFactoryType();
    is_aat_task = (ft == TaskFactoryType::AAT || ft == TaskFactoryType::MAT);

    if (is_aat_task)
      aat_min_time = task->GetOrderedTaskSettings().aat_min_time;

    for (unsigned i = 0; i < task->TaskSize(); i++)
      turnpoints.push_back(task->GetTaskPoint(i));
  }
}
