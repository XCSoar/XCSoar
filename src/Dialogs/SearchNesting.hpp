// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Browse vs pick-only close rules for a list that opens a child
 * details dialog.  See `.cursor/rules/search-dialog-ux.mdc`.
 *
 * The outer list stays open until the user cancels it or the child
 * sets #state_change_committed (Goto, task, ack, and so on).
 */
struct DetailsNesting {
  bool *state_change_committed = nullptr;

  /** If false, pan does not set #state_change_committed. */
  bool include_pan_in_parent_dismissal = true;

  /**
   * If non-null, set true when the user panned from details,
   * regardless of #include_pan_in_parent_dismissal.
   */
  bool *map_pan_from_details = nullptr;
};

/** Waypoint details use the shared browse/pick contract. */
using WaypointDetailsNesting = DetailsNesting;
