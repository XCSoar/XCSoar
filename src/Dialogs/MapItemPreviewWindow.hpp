// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Look/MapLook.hpp"
#include "Look/TrafficLook.hpp"
#include "MapWindow/Preview/MapPreviewWindow.hpp"

/**
 * Map-elements list preview: terrain / topography / airspace plus item
 * overlays (airspace outline, task leg, traffic symbol).
 */
class MapItemPreviewWindow final : public MapPreviewWindow {
  const TrafficLook &traffic_look;
  const TaskLook &task_look;
  const WaypointLook &waypoint_look;

public:
  MapItemPreviewWindow(const MapLook &map_look,
                       const TrafficLook &traffic_look) noexcept
    :MapPreviewWindow(map_look.airspace, map_look.topography, nullptr),
     traffic_look(traffic_look),
     task_look(map_look.task),
     waypoint_look(map_look.waypoint)
  {
    SetAirspacePreviewMapLook(&map_look);
  }

protected:
  void UpdateProjection() noexcept override;
  void PaintOverlays(Canvas &canvas) noexcept override;
};
