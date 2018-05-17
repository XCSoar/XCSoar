/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_RENDER_TASK_POINT_HPP
#define XCSOAR_RENDER_TASK_POINT_HPP

#include "Geo/GeoPoint.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"
#include "MapWindow/MapCanvas.hpp"
#include "Compiler.h"

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

  gcc_pure
  bool IsTargetVisible(const TaskPoint &tp) const;

  void DrawBearing(const TaskPoint &tp);
  void DrawTarget(const TaskPoint &tp);
  void DrawTaskLine(const GeoPoint &start, const GeoPoint &end);
  void DrawIsoline(const AATPoint &tp);
  void DrawOZBackground(Canvas &canvas, const OrderedTaskPoint &tp,
                        int offset);
  void DrawOZForeground(const OrderedTaskPoint &tp, int offset);
};

#endif
