// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Ptr.hpp"

struct GeoPoint;
struct Waypoint;
class Waypoints;
class OrderedTask;

WaypointPtr
ShowWaypointListDialog(const GeoPoint &location,
                       OrderedTask *ordered_task = nullptr,
                       unsigned ordered_task_index = 0);

void
dlgConfigWaypointsShowModal();

enum class WaypointEditResult {
  /** editing was canceled by the user */
  CANCEL,

  /** the object was modified by the user */
  MODIFIED,

  /** the user has confirmed, but has not modified anything */
  UNMODIFIED,
};

/**
 * @return true if the given #Waypoint was modified
 */
WaypointEditResult
dlgWaypointEditShowModal(Waypoint &way_point);

void
dlgWaypointDetailsShowModal(WaypointPtr waypoint,
                            bool allow_navigation = true,
                            bool allow_edit = false);

bool
PopupNearestWaypointDetails(const Waypoints &way_points,
                            const GeoPoint &location,
                            double range);
