// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapWindow/Preview/MapPreviewWindow.hpp"

struct AirspaceLook;
struct TaskLook;
struct TopographyLook;

/**
 * Turn Points tab map preview: terrain stack plus read-only ordered task
 * geometry (#MapPreviewFocusTaskTurnpoint / #MapPreviewFocusTaskWhole).
 */
class TaskEditMapPreviewWindow final : public MapPreviewWindow {
  const TaskLook &task_look;

public:
  TaskEditMapPreviewWindow(const AirspaceLook &airspace_look,
                           const TopographyLook &topography_look,
                           const TaskLook &_task_look) noexcept
    :MapPreviewWindow(airspace_look, topography_look, nullptr),
     task_look(_task_look) {}

protected:
  void PaintOverlays(Canvas &canvas) noexcept override;
};
