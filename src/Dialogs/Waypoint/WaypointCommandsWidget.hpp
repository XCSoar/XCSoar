// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "Engine/Waypoint/Ptr.hpp"

class WndForm;
class Waypoints;
class ProtectedTaskManager;

/**
 * A Widget that shows a few commands for a Waypoint.
 */
class WaypointCommandsWidget final
  : public RowFormWidget {
  WndForm *const form;

  Waypoints *const waypoints;

  const WaypointPtr waypoint;

  ProtectedTaskManager *const task_manager;

  const bool allow_edit;
  
  bool has_freq;
  enum Buttons {
    REPLACE_IN_TASK,
    INSERT_IN_TASK,
    APPEND_TO_TASK,
    REMOVE_FROM_TASK,
	SET_NEW_HOME,
	PAN_WAYPOINT,
	SET_ACTIVE_FREQUENCY,
	SET_STANDBY_FREQUENCY,
	EDIT,
  };

  Button *replace_button, *insert_button, *append_button, *remove_button, *home_button, *pan_button, *set_active_button, *set_standby_button, *edit_button;

public:
  WaypointCommandsWidget(const DialogLook &look, WndForm *_form,
                         Waypoints *_waypoints,
                         WaypointPtr _waypoint,
                         ProtectedTaskManager *_task_manager,
                         bool _allow_edit) noexcept
    :RowFormWidget(look), form(_form),
     waypoints(_waypoints),
     waypoint(std::move(_waypoint)), task_manager(_task_manager),
     allow_edit(_allow_edit) {}
  void UpdateButtons();
  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  
};
