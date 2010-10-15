/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Projection.hpp"
#include "Task/Tasks/BaseTask/UnorderedTaskPoint.hpp"
#include "Task/TaskPoints/AATIsolineSegment.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Math/Screen.hpp"
#include "RenderObservationZone.hpp"
#include "NMEA/Info.hpp"
#include "SettingsMap.hpp"

RenderTaskPoint::RenderTaskPoint(Canvas &_canvas,
                                 const Projection &_projection,
                                 const SETTINGS_MAP &_settings_map,
                                 RenderObservationZone &_ozv,
                                 const bool draw_bearing,
                                 const GeoPoint &location)
  :m_canvas(_canvas), m_buffer(_canvas), m_proj(_projection),
   map_canvas(_canvas, _projection),
   m_settings_map(_settings_map),
   m_draw_bearing(draw_bearing),
   pen_leg_active(Pen::DASH, IBLSCALE(2), Graphics::TaskColor),
   pen_leg_inactive(Pen::DASH, IBLSCALE(1), Graphics::TaskColor),
   pen_leg_arrow(Pen::SOLID, IBLSCALE(1), Graphics::TaskColor),
   pen_isoline(Pen::SOLID, IBLSCALE(2), Color(0,0,255)), 
   m_index(0),
   ozv(_ozv),
   m_active_index(0),
   m_layer(0),
   m_location(location)
{
}

void 
RenderTaskPoint::set_layer(unsigned set) 
{
  m_layer = set;
  m_index = 0;
}

void 
RenderTaskPoint::Visit(const UnorderedTaskPoint& tp) 
{
  if (m_layer == 1) {
    draw_task_line(m_location, tp.get_location_remaining());
  }
  if (m_layer == 3) {
    draw_bearing(tp);
    if (m_settings_map.EnableDetourCostMarker)
      draw_off_track(tp);
  }
  m_index++;
}

void 
RenderTaskPoint::draw_ordered(const OrderedTaskPoint& tp) 
{
  if (m_layer == 0) {
    // draw shaded part of observation zone
    draw_oz_background(tp);

    // Draw clear area on top indicating part of OZ already travelled in.
    // This provides a simple and intuitive visual representation of
    // where in the OZ to go to increase scoring distance.

    draw_samples(tp);
  }
  
  if (m_layer == 1) {
    if (m_index>0) {
      draw_task_line(m_last_point, tp.get_location_remaining());
    }
    m_last_point = tp.get_location_remaining();
  }
  
  if (m_layer == 2) {
    draw_oz_foreground(tp);
  }
}

void 
RenderTaskPoint::Visit(const StartPoint& tp) 
{
  m_index = 0;
  draw_ordered(tp);
  if (m_layer == 3) {
    draw_bearing(tp);
    draw_target(tp);
  }
}

void 
RenderTaskPoint::Visit(const FinishPoint& tp) 
{
  m_index++;
  draw_ordered(tp);
  if (m_layer == 3) {
    draw_bearing(tp);
    if (m_settings_map.EnableDetourCostMarker)
      draw_off_track(tp);
    draw_target(tp);
  }
}

void 
RenderTaskPoint::Visit(const AATPoint& tp) 
{
  m_index++;
  
  draw_ordered(tp);
  if (m_layer == 3) {
    draw_isoline(tp);
    draw_bearing(tp);
    if (m_settings_map.EnableDetourCostMarker)
      draw_off_track(tp);
    draw_target(tp);
  }
}

void 
RenderTaskPoint::Visit(const ASTPoint& tp) 
{
  m_index++;
  
  draw_ordered(tp);
  if (m_layer == 3) {
    draw_bearing(tp);
    if (m_settings_map.EnableDetourCostMarker)
      draw_off_track(tp);
    draw_target(tp);
  }
}

void 
RenderTaskPoint::set_active_index(unsigned active_index) 
{
  m_active_index = active_index;
}

bool 
RenderTaskPoint::leg_active() 
{
  return (m_index+1>m_active_index);
}

bool 
RenderTaskPoint::point_past() 
{
  return (m_index<m_active_index);
}

bool 
RenderTaskPoint::point_current() 
{
  return (m_index==m_active_index);
}

bool 
RenderTaskPoint::do_draw_samples(const TaskPoint& tp) 
{
  return point_current() || point_past();
}

bool 
RenderTaskPoint::do_draw_bearing(const TaskPoint &tp) 
{
  return m_draw_bearing && point_current();
}

bool 
RenderTaskPoint::do_draw_target(const TaskPoint &tp) 
{
  if (!tp.has_target()) {
    return false;
  }
  return (point_current()
          || m_settings_map.EnablePan 
          || m_settings_map.TargetPan);
}

bool 
RenderTaskPoint::do_draw_isoline(const TaskPoint &tp) 
{
  return do_draw_target(tp);
}

void 
RenderTaskPoint::draw_bearing(const TaskPoint &tp) 
{
  if (!do_draw_bearing(tp)) 
    return;

  m_buffer.select(Graphics::hpBearing);
  map_canvas.line(m_location, tp.get_location_remaining());
}

void 
RenderTaskPoint::draw_target(const TaskPoint &tp) 
{
  if (!do_draw_target(tp)) 
    return;

/*  
    POINT sc;
    if (map.LonLat2ScreenIfVisible(tp.get_location_remaining(), &sc)) {
    Graphics::hBmpTarget.draw(get_canvas(), get_bitmap_canvas(), sc.x, sc.y);
    }
*/
}

void 
RenderTaskPoint::draw_task_line(const GeoPoint& start, const GeoPoint& end) 
{
  m_buffer.select(leg_active() ? pen_leg_active : pen_leg_inactive);
  m_buffer.background_transparent();
  map_canvas.line(start, end);
  m_buffer.background_opaque();
  
  // draw small arrow along task direction
  POINT p_p;
  POINT Arrow[3] = { {6,6}, {-6,6}, {0,0} };
  
  const POINT p_start = m_proj.LonLat2Screen(start);
  const POINT p_end = m_proj.LonLat2Screen(end);
  
  const Angle ang = Angle::radians(atan2(fixed(p_end.x - p_start.x),
                                         fixed(p_start.y - p_end.y))).as_bearing();

  ScreenClosestPoint(p_start, p_end, m_proj.GetOrigScreen(), &p_p, IBLSCALE(25));
  PolygonRotateShift(Arrow, 2, p_p.x, p_p.y, ang);
  Arrow[2] = Arrow[1];
  Arrow[1] = p_p;
  
  m_buffer.select(pen_leg_arrow);
  m_buffer.polyline(Arrow, 3);
}

void 
RenderTaskPoint::draw_isoline(const AATPoint& tp) 
{
  if (!tp.valid() || !do_draw_isoline(tp))
    return;

  AATIsolineSegment seg(tp);
  if (!seg.valid()) {
    return;
  }
  std::vector<POINT> screen; 
  static const fixed fixed_twentieth(1.0 / 20.0);
  
  if (m_proj.DistanceMetersToScreen(seg.parametric(fixed_zero).
                                    distance(seg.parametric(fixed_one)))>2) {
    
    for (fixed t = fixed_zero; t<=fixed_one; t+= fixed_twentieth) {
      GeoPoint ga = seg.parametric(t);
      screen.push_back(m_proj.LonLat2Screen(ga));
    }
    if (screen.size()>=2) {
      m_buffer.select(pen_isoline);
      m_buffer.polyline(&screen[0], screen.size());
    }
  }
}

void 
RenderTaskPoint::draw_samples(const OrderedTaskPoint& tp) 
{
  if (!do_draw_samples(tp)) {
    return;
  }
  /*
    m_buffer.set_text_color(Graphics::Colours[m_settings_map.
    iAirspaceColour[1]]);
    // get brush, can be solid or a 1bpp bitmap
    m_buffer.select(Graphics::hAirspaceBrushes[m_settings_map.
    iAirspaceBrush[1]]);
    */
    // erase where aircraft has been
    m_buffer.white_brush();
    m_buffer.white_pen();
    
  map_canvas.draw(tp.get_sample_points());
}

void 
RenderTaskPoint::draw_oz_background(const OrderedTaskPoint& tp) 
{
  ozv.set_past(point_past());
  ozv.set_current(point_current());
  ozv.set_background(true);
  ((ObservationZoneConstVisitor &)ozv).Visit(*tp.get_oz());
}

void 
RenderTaskPoint::draw_oz_foreground(const OrderedTaskPoint& tp) 
{
  ozv.set_past(point_past());
  ozv.set_current(point_current());
  ozv.set_background(false);
  ((ObservationZoneConstVisitor &)ozv).Visit(*tp.get_oz());
}

void 
RenderTaskPoint::draw_off_track(const TaskPoint &tp) 
{

}
