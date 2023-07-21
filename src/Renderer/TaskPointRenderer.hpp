// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"
#include "MapWindow/MapCanvas.hpp"

#include <cstdint>

class Canvas;
class WindowProjection;
class OZRenderer;
class TaskPoint;
class OrderedTaskPoint;
class AATPoint;
class FlatProjection;
struct TaskLook;

class TaskPointRenderer
{
public:
  enum class TargetVisibility : uint_least8_t {
    ALL,
    ACTIVE,
    NONE,
  };

private:
  Canvas &canvas;
  const WindowProjection &m_proj;
  MapCanvas map_canvas;
  const TaskLook &task_look;
  const FlatProjection &flat_projection;

  GeoPoint last_point;
  OZRenderer &ozv;
  const GeoPoint location;
  FlatBoundingBox bb_screen;

  unsigned index = 0;
  unsigned active_index = 0;

  const TargetVisibility target_visibility;
  const bool draw_bearing;

  bool task_finished = false;
  bool mode_optional_start = false;

public:
  enum class Layer : uint_least8_t {
    OZ_SHADE,
    LEG,
    OZ_OUTLINE,
    SYMBOLS,
  };

  /**
   * @param aircraft_location the aircraft's location or
   * GeoPoint::Invalid()
   */
  TaskPointRenderer(Canvas &_canvas,
                    const WindowProjection &_projection,
                    const TaskLook &task_look,
                    const FlatProjection &_flat_projection,
                    OZRenderer &_ozv,
                    bool _draw_bearing,
                    TargetVisibility _target_visibility,
                    const GeoPoint &aircraft_location) noexcept;

  void ResetIndex() noexcept {
    index = 0;
  }

  void SetActiveIndex(unsigned _active_index) noexcept {
    active_index = _active_index;
  }

  void SetBoundingBox(const FlatBoundingBox &bb) noexcept {
    bb_screen = bb;
  }

  void SetTaskFinished(bool _task_finished) noexcept {
    task_finished = _task_finished;
  }

  void SetModeOptional(const bool mode) noexcept {
    mode_optional_start = mode;
  }

  void Draw(const TaskPoint &tp, Layer layer) noexcept;

private:
  void DrawOrdered(const OrderedTaskPoint &tp, Layer layer) noexcept;

  bool LegActive() const noexcept {
    return index >= active_index;
  }

  bool PointPast() const noexcept {
    return index < active_index;
  }

  bool PointCurrent() const noexcept {
    return index == active_index;
  }

  [[gnu::pure]]
  bool IsTargetVisible(const TaskPoint &tp) const noexcept;

  void DrawBearing(const TaskPoint &tp) noexcept;
  void DrawTarget(const TaskPoint &tp) noexcept;
  void DrawTaskLine(const GeoPoint &start, const GeoPoint &end) noexcept;
  void DrawIsoline(const AATPoint &tp) noexcept;
  void DrawOZBackground(Canvas &canvas, const OrderedTaskPoint &tp,
                        int offset) noexcept;
  void DrawOZForeground(const OrderedTaskPoint &tp, int offset) noexcept;
};
