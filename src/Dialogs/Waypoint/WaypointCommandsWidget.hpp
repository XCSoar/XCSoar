// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "Engine/Waypoint/Ptr.hpp"

class WndForm;
class ProtectedTaskManager;

/**
 * A Widget that shows a few commands for a Waypoint.
 */
class WaypointCommandsWidget final
  : public RowFormWidget {
  WndForm *const form;

  const WaypointPtr waypoint;

  ProtectedTaskManager *const task_manager;

  const bool allow_edit;

public:
  WaypointCommandsWidget(const DialogLook &look, WndForm *_form,
                         WaypointPtr _waypoint,
                         ProtectedTaskManager *_task_manager,
                         bool _allow_edit) noexcept
    :RowFormWidget(look), form(_form),
     waypoint(std::move(_waypoint)), task_manager(_task_manager),
     allow_edit(_allow_edit) {}

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
