// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Renderer/TaskPointRenderer.hpp"

#include <optional>

class Canvas;
class WindowProjection;
struct TaskLook;
class FlatProjection;
class AirspaceRenderer;
struct MapSettings;
class AbstractTask;

/**
 * Shared setup for drawing the active navigatable task on any map-like
 * projection (main map, embedded previews, target dialog, ordered-task
 * previews).  Caller performs validation gates (e.g.
 * IsLiveTaskPreviewValid) and bearing/route overlays separately.
 *
 * @param active_index optional highlighted task point index; map-item OZ preview
 * and task-edit turnpoint preview pass an index, task-edit whole-task and main
 * map use default nullopt.
 */
void
DrawActiveTaskForProjection(Canvas &canvas,
                            const WindowProjection &projection,
                            const TaskLook &task_look,
                            const AbstractTask &task,
                            const FlatProjection &flat_projection,
                            const AirspaceRenderer &airspace_renderer,
                            const MapSettings &map_settings,
                            bool draw_bearing,
                            TaskPointRenderer::TargetVisibility target_visibility,
                            GeoPoint reference_location,
                            bool task_finished,
                            std::optional<unsigned> active_index =
                              std::nullopt) noexcept;
