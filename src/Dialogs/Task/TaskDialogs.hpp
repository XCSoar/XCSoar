// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Alternate.hpp"
#include "Engine/Waypoint/Ptr.hpp"

#include <memory>
#include <optional>

class OrderedTask;
class Waypoints;

/**
 * @param task if set, load this task into the dialog; if not set,
 * show the current task
 */
void
dlgTaskManagerShowModal(std::unique_ptr<OrderedTask> task);

void
dlgTaskManagerShowModal();

/**
 * Show a dialog that lets the user edit a task point (and lets him
 * navigate to other task points).
 *
 * @return true if the task was modified
 */
bool
dlgTaskPointShowModal(OrderedTask &task, const unsigned index);

/**
 * Show a dialog that lets the user mutate one task point to another
 * type.
 *
 * @return true if the task was modified
 */
bool
dlgTaskPointType(OrderedTask &task, unsigned index);

bool
dlgTaskOptionalStarts(Waypoints &waypoints, OrderedTask &task);

/**
 * Shows map display zoomed to target point
 * with half dialog popup to manipulate point
 *
 * @param TargetPoint if -1 then goes to active target
 * else goes to TargetPoint by default
 */
void
dlgTargetShowModal(int TargetPoint = -1);

/**
 * Shows the current alternates list.
 *
 * @param slot if set, the dialog exposes the manual/auto controls for
 * the specified alternate InfoBox slot; otherwise it behaves as a
 * generic alternates list dialog
 */
void
dlgAlternatesListShowModal(Waypoints *waypoints,
                           std::optional<AlternateInfoBoxSlot> slot =
                             std::nullopt) noexcept;

/**
 * Shows the current alternates list and returns the selected waypoint.
 *
 * This is the generic selection flow used by the manual alternate
 * feature and does not expose slot-specific controls.
 */
WaypointPtr
dlgAlternatesListSelectWaypoint() noexcept;
