// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Ptr.hpp"
#include "Waypoint/WaypointFilter.hpp"

#include <optional>

struct GeoPoint;
struct Waypoint;
class Waypoints;
class OrderedTask;

/**
 * Open the modal Waypoint Selection dialog.
 *
 * @param initial_type If set, force the Type filter dropdown to the
 * given value on entry and clear the name/distance/direction filters
 * so the user sees a focused list (e.g. only the recently-used
 * waypoints).  When unset, the dialog opens with whatever filter
 * state was left by the previous invocation, except the name field
 * which is always cleared.  Used by ``event=GotoLookup <misc>`` to
 * jump straight to a category.
 *
 * @param prepopulate_with_task If true and @p ordered_task is non-null,
 * open the list pre-populated with the task's waypoints in task order
 * (with a leading "Choose a filter" prompt row), instead of the
 * filter-driven full database.  Used by the Active Waypoint InfoBox
 * tap action.
 *
 * @param action_row_label Optional non-waypoint row rendered at the top of
 * the list (using the same style as the filter prompt). When set, it
 * replaces the "Choose a filter or click here" prompt.
 * @param action_selected Optional out-param. When non-null and the user
 * activated the action row, set to true; on return, @p action_selected
 * lets the caller distinguish "user picked the action row" from
 * "user picked a waypoint" / "cancelled".
 */
WaypointPtr
ShowWaypointListDialog(Waypoints &waypoints, const GeoPoint &location,
                       OrderedTask *ordered_task = nullptr,
                       unsigned ordered_task_index = 0,
                       std::optional<TypeFilter> initial_type = std::nullopt,
                       bool prepopulate_with_task = false,
                       const char *action_row_label = nullptr,
                       bool *action_selected = nullptr);
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

/**
 * Forward the ``WaypointImage`` input event to an open waypoint-details
 * dialog (embedded image only).  No effect if the dialog is not on an
 * image page.
 */
void
WaypointDetailsDispatchImageInput(const char *misc) noexcept;

bool
PopupNearestWaypointDetails(Waypoints &waypoints,
                            const GeoPoint &location,
                            double range);
