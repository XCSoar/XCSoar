/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#if !defined(XCSOAR_RENDER_TASK_POINT_HPP)
#define XCSOAR_RENDER_TASK_POINT_HPP

#include "Navigation/GeoPoint.hpp"
#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Screen/Pen.hpp"
#include "MapWindow/MapCanvas.hpp"
#include "Compiler.h"

class Canvas;
class WindowProjection;
class RenderObservationZone;
class OrderedTaskPoint;
struct SETTINGS_MAP;

enum RenderTaskLayer {
  RENDER_TASK_OZ_SHADE,
  RENDER_TASK_LEG,
  RENDER_TASK_OZ_OUTLINE,
  RENDER_TASK_SYMBOLS,
};

class RenderTaskPoint:
  public TaskPointConstVisitor
{
protected:
  Canvas &canvas, *buffer;
  const WindowProjection &m_proj;
  MapCanvas map_canvas;
  const SETTINGS_MAP &m_settings_map;
  const TaskProjection &task_projection;

public:
  RenderTaskPoint(Canvas &_canvas, Canvas *_buffer,
                  const WindowProjection &_projection,
                  const SETTINGS_MAP &_settings_map,
                  const TaskProjection &_task_projection,
                  RenderObservationZone &_ozv,
                  const bool draw_bearing,
                  const GeoPoint &location);

  void Visit(const UnorderedTaskPoint& tp);
  void Visit(const StartPoint& tp);
  void Visit(const FinishPoint& tp);
  void Visit(const AATPoint& tp);
  void Visit(const ASTPoint& tp);

  void set_layer(RenderTaskLayer set) {
    m_layer = set;
    m_index = 0;
  }

  void set_active_index(unsigned active_index) {
    m_active_index = active_index;
  }

  void set_bounding_box(const FlatBoundingBox& bb) {
    bb_screen = bb;
  }

  void set_mode_optional(const bool mode) {
    mode_optional_start = mode;
  }

protected:
  void draw_ordered(const OrderedTaskPoint& tp);

  bool leg_active() const {
    return m_index >= m_active_index;
  }

  bool point_past() const {
    return m_index < m_active_index;
  }

  bool point_current() const {
    return m_index == m_active_index;
  }

  bool do_draw_deadzone(const TaskPoint& tp) const {
    return point_current() || point_past();
  }

  bool do_draw_bearing(const TaskPoint &tp) const {
    return m_draw_bearing && point_current();
  }

  gcc_pure
  bool do_draw_target(const TaskPoint &tp) const;

  bool do_draw_isoline(const TaskPoint &tp) const {
    return do_draw_target(tp);
  }

  void draw_bearing(const TaskPoint &tp);
  virtual void draw_target(const TaskPoint &tp);
  void draw_task_line(const GeoPoint& start, const GeoPoint& end);
  void draw_isoline(const AATPoint& tp);
  /**
   * Clear the part of the OZ background (shaded area) over which
   * the aircraft has flown.
   * The samples polygon is a convex hull, flying inside the polygon
   * cannot possibly result in greater scored distance.
   *
   * @param tp
   */
  void draw_deadzone(const AATPoint& tp);
  void draw_oz_background(Canvas &canvas, const OrderedTaskPoint& tp);
  void draw_oz_foreground(const OrderedTaskPoint& tp);
  const bool m_draw_bearing;
  GeoPoint m_last_point;
  unsigned m_index;
  RenderObservationZone &ozv;
  unsigned m_active_index;
  RenderTaskLayer m_layer;
  const GeoPoint m_location;
  FlatBoundingBox bb_screen;
  bool mode_optional_start;
};


#endif
