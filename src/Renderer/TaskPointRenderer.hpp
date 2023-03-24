// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"
#include "MapWindow/MapCanvas.hpp"

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
  enum TargetVisibility {
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

  const bool draw_bearing;
  TargetVisibility target_visibility;

  GeoPoint last_point;
  unsigned index;
  OZRenderer &ozv;
  unsigned active_index;
  const GeoPoint location;
  FlatBoundingBox bb_screen;
  bool task_finished;
  bool mode_optional_start;

public:
  enum Layer {
    LAYER_OZ_SHADE,
    LAYER_LEG,
    LAYER_OZ_OUTLINE,
    LAYER_SYMBOLS,
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
                    const GeoPoint &aircraft_location);

  void ResetIndex() {
    index = 0;
  }

  void SetActiveIndex(unsigned _active_index) {
    active_index = _active_index;
  }

  void SetBoundingBox(const FlatBoundingBox &bb) {
    bb_screen = bb;
  }

  void SetTaskFinished(bool _task_finished) {
    task_finished = _task_finished;
  }

  void SetModeOptional(const bool mode) {
    mode_optional_start = mode;
  }

  void Draw(const TaskPoint &tp, Layer layer);

private:
  void DrawOrdered(const OrderedTaskPoint &tp, Layer layer);

  bool LegActive() const {
    return index >= active_index;
  }

  bool PointPast() const {
    return index < active_index;
  }

  bool PointCurrent() const {
    return index == active_index;
  }

  [[gnu::pure]]
  bool IsTargetVisible(const TaskPoint &tp) const;

  void DrawBearing(const TaskPoint &tp);
  void DrawTarget(const TaskPoint &tp);
  void DrawTaskLine(const GeoPoint &start, const GeoPoint &end);
  void DrawIsoline(const AATPoint &tp);
  void DrawOZBackground(Canvas &canvas, const OrderedTaskPoint &tp,
                        int offset);
  void DrawOZForeground(const OrderedTaskPoint &tp, int offset);
};
