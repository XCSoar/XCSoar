// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Ptr.hpp"

struct GeoPoint;
struct Waypoint;
class Waypoints;
class OrderedTask;

WaypointPtr
ShowWaypointListDialog(Waypoints &waypoints, const GeoPoint &location,
                       OrderedTask *ordered_task = nullptr,
                       unsigned ordered_task_index = 0);
void
ShowWaypointListPersistentDialog(
  const GeoPoint &location, bool allow_navigation = true,
  bool allow_edit = true);

void
dlgConfigWaypointsShowModal(Waypoints &waypoints) noexcept;

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

/**
 * Optional coordination between nested \ref dlgWaypointDetailsShowModal and a
 * parent list dialog. All pointers, when non-null, refer to the caller; on
 * open, the modal assigns @c false to the locations pointed to by
 * @c state_change_committed and @c map_pan_from_details when they are
 * non-null.
 */
struct WaypointDetailsNesting {
  bool *state_change_committed = nullptr;

  /** If false, pan (header or commands) does not set #state_change_committed. */
  bool include_pan_in_parent_dismissal = true;

  /**
   * If non-null, set to true when the user panned the map to the waypoint
   * (Pan To / Pan to Waypoint) before leaving details, regardless of
   * #include_pan_in_parent_dismissal.
   */
  bool *map_pan_from_details = nullptr;
};

void
dlgWaypointDetailsShowModal(
  Waypoints *waypoints, WaypointPtr waypoint, bool allow_navigation = true,
  bool allow_edit = false,
  const WaypointDetailsNesting *nesting = nullptr) noexcept;

/**
 * Run #dlgWaypointDetailsShowModal with #WaypointDetailsNesting and only
 * #state_change_committed set, for a parent list in browse mode.
 *
 * @return true if the user committed a state-changing action in details
 * (the parent may dismiss with #mrOK when so).
 */
[[nodiscard]] bool
dlgWaypointDetailsShowModalForBrowseParent(
  Waypoints *waypoints, WaypointPtr &&waypoint, bool allow_navigation,
  bool allow_edit) noexcept;

bool
PopupNearestWaypointDetails(Waypoints &waypoints,
                            const GeoPoint &location,
                            double range);
