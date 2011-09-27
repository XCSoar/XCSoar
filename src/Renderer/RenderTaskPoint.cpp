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

#include "RenderTaskPoint.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "WindowProjection.hpp"
#include "Task/Tasks/BaseTask/UnorderedTaskPoint.hpp"
#include "Task/TaskPoints/AATIsolineSegment.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Look/TaskLook.hpp"
#include "Math/Screen.hpp"
#include "OZRenderer.hpp"

RenderTaskPoint::RenderTaskPoint(Canvas &_canvas,
                                 const WindowProjection &_projection,
                                 const TaskLook &_task_look,
                                 const TaskProjection &_task_projection,
                                 OZRenderer &_ozv,
                                 bool _draw_bearing,
                                 bool _draw_all,
                                 const GeoPoint &_location)
  :canvas(_canvas), m_proj(_projection),
   map_canvas(_canvas, _projection,
              _projection.GetScreenBounds().scale(fixed(1.1))),
   task_look(_task_look),
   task_projection(_task_projection),
   draw_bearing(_draw_bearing),
   draw_all(_draw_all),
   index(0),
   ozv(_ozv),
   active_index(0),
   location(_location),
   mode_optional_start(false)
{
}

void 
RenderTaskPoint::DrawOrdered(const OrderedTaskPoint &tp, Layer layer)
{
  if (layer == LAYER_OZ_SHADE && tp.boundingbox_overlaps(bb_screen))
    // draw shaded part of observation zone
    DrawOZBackground(canvas, tp);
  
  if (layer == LAYER_LEG) {
    if (index > 0)
      DrawTaskLine(last_point, tp.GetLocationRemaining());

    last_point = tp.GetLocationRemaining();
  }
  
  if (layer == LAYER_OZ_OUTLINE && tp.boundingbox_overlaps(bb_screen))
    DrawOZForeground(tp);
}

bool 
RenderTaskPoint::DoDrawTarget(const TaskPoint &tp) const
{
  if (!tp.HasTarget())
    return false;

  return draw_all || PointCurrent();
}

void 
RenderTaskPoint::DrawBearing(const TaskPoint &tp) 
{
  if (!DoDrawBearing(tp)) 
    return;

  canvas.select(task_look.bearing_pen);
  map_canvas.offset_line(location, tp.GetLocationRemaining());
}

void 
RenderTaskPoint::DrawTarget(const TaskPoint &tp) 
{
}

void 
RenderTaskPoint::DrawTaskLine(const GeoPoint &start, const GeoPoint &end)
{
  canvas.select(LegActive() ? task_look.leg_active_pen :
                              task_look.leg_inactive_pen);
  canvas.background_transparent();
  map_canvas.line(start, end);
  canvas.background_opaque();
  
  // draw small arrow along task direction
  RasterPoint p_p;
  RasterPoint Arrow[3] = { {6,6}, {-6,6}, {0,0} };
  
  const RasterPoint p_start = m_proj.GeoToScreen(start);
  const RasterPoint p_end = m_proj.GeoToScreen(end);
  
  const Angle ang = Angle::radians(atan2(fixed(p_end.x - p_start.x),
                                         fixed(p_start.y - p_end.y))).as_bearing();

  ScreenClosestPoint(p_start, p_end, m_proj.GetScreenOrigin(), &p_p, Layout::Scale(25));
  PolygonRotateShift(Arrow, 2, p_p.x, p_p.y, ang);
  Arrow[2] = Arrow[1];
  Arrow[1] = p_p;
  
  canvas.select(task_look.arrow_pen);
  canvas.polyline(Arrow, 3);
}

void 
RenderTaskPoint::DrawIsoline(const AATPoint &tp)
{
  if (!tp.valid() || !DoDrawIsoline(tp))
    return;

  AATIsolineSegment seg(tp, task_projection);
  if (!seg.IsValid())
    return;

  #define fixed_twentieth fixed(1.0 / 20.0)
  
  if (m_proj.GeoToScreenDistance(seg.Parametric(fixed_zero).
                                    Distance(seg.Parametric(fixed_one))) > 2) {
    RasterPoint screen[21];
    for (unsigned i = 0; i < 21; ++i) {
      fixed t = i * fixed_twentieth;
      GeoPoint ga = seg.Parametric(t);
      screen[i] = m_proj.GeoToScreen(ga);
    }

    canvas.select(task_look.isoline_pen);
    canvas.background_transparent();
    canvas.polyline(screen, 21);
    canvas.background_opaque();
  }
}

void
RenderTaskPoint::DrawOZBackground(Canvas &canvas, const OrderedTaskPoint &tp)
{
  ozv.Draw(canvas, OZRenderer::LAYER_SHADE, m_proj, *tp.get_oz(),
           index - active_index);
}

void 
RenderTaskPoint::DrawOZForeground(const OrderedTaskPoint &tp)
{
  int offset = index - active_index;
  if (mode_optional_start)
    offset = -1; // render optional starts as deactivated

  ozv.Draw(canvas, OZRenderer::LAYER_INACTIVE, m_proj, *tp.get_oz(),
           offset);
  ozv.Draw(canvas, OZRenderer::LAYER_ACTIVE, m_proj, *tp.get_oz(),
           offset);
}

void
RenderTaskPoint::Draw(const TaskPoint &tp, Layer layer)
{
  const OrderedTaskPoint &otp = (const OrderedTaskPoint &)tp;
  const AATPoint &atp = (const AATPoint &)tp;

  switch (tp.GetType()) {
  case TaskPoint::UNORDERED:
    if (layer == LAYER_LEG)
      DrawTaskLine(location, tp.GetLocationRemaining());

    if (layer == LAYER_SYMBOLS)
      DrawBearing(tp);

    index++;
    break;

  case TaskPoint::START:
    index = 0;

    DrawOrdered(otp, layer);
    if (layer == LAYER_SYMBOLS) {
      DrawBearing(tp);
      DrawTarget(tp);
    }

    break;

  case TaskPoint::AST:
    index++;

    DrawOrdered(otp, layer);
    if (layer == LAYER_SYMBOLS) {
      DrawBearing(tp);
      DrawTarget(tp);
    }
    break;

  case TaskPoint::AAT:
    index++;

    DrawOrdered(otp, layer);
    if (layer == LAYER_SYMBOLS) {
      DrawIsoline(atp);
      DrawBearing(tp);
      DrawTarget(tp);
    }
    break;

  case TaskPoint::FINISH:
    index++;

    DrawOrdered(otp, layer);
    if (layer == LAYER_SYMBOLS) {
      DrawBearing(tp);
      DrawTarget(tp);
    }
    break;

  case TaskPoint::ROUTE:
    /* unreachable */
    assert(false);
    break;
  }
}
