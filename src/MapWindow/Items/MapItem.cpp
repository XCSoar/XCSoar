// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapItem.hpp"
#include "Engine/Task/ObservationZones/ObservationZonePoint.hpp"

TaskOZMapItem::TaskOZMapItem(int _index, const ObservationZonePoint &_oz,
                             TaskPointType _tp_type, WaypointPtr &&_waypoint)
  :MapItem(Type::TASK_OZ), index(_index), oz(_oz.Clone()),
   tp_type(_tp_type), waypoint(std::move(_waypoint)) {}

TaskOZMapItem::~TaskOZMapItem() noexcept = default;
