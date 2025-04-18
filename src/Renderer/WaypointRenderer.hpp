// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/NonCopyable.hpp"

struct WaypointRendererSettings;
struct WaypointLook;
class Canvas;
class LabelBlock;
class MapWindowProjection;
class Waypoints;
struct PolarSettings;
struct TaskBehaviour;
struct MoreData;
struct DerivedInfo;
class ProtectedTaskManager;
class ProtectedRoutePlanner;

/**
 * Renders way point icons and labels into a #Canvas.
 */
class WaypointRenderer : private NonCopyable {
  const Waypoints *way_points;

  const WaypointLook &look;

public:
  WaypointRenderer(const Waypoints *_way_points,
                   const WaypointLook &_look) noexcept
    :way_points(_way_points), look(_look) {}

  const WaypointLook &GetLook() const noexcept {
    return look;
  }

  void SetWaypoints(const Waypoints *_way_points) noexcept {
    way_points = _way_points;
  }

  void Render(Canvas &canvas, LabelBlock &label_block,
              const MapWindowProjection &projection,
              const WaypointRendererSettings &settings,
              const PolarSettings &polar_settings,
              const TaskBehaviour &task_behaviour,
              const MoreData &basic, const DerivedInfo &calculated,
              const ProtectedTaskManager *task,
              const ProtectedRoutePlanner *route_planner) noexcept;
};
