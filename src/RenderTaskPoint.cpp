
#include "RenderTaskPoint.hpp"
#include "Screen/Graphics.hpp"
#include "Projection.hpp"
#include "Task/TaskPoints/AATIsolineSegment.hpp"
#include "Math/Geometry.hpp"
#include "Math/Screen.hpp"
#include "RenderObservationZone.hpp"
#include "NMEA/Info.hpp"

RenderTaskPoint::RenderTaskPoint(MapDrawHelper &_helper,
                                 RenderObservationZone &_ozv,
                                 const bool draw_bearing,
                                 const GEOPOINT location)
  :MapDrawHelper(_helper),
   m_draw_bearing(draw_bearing),
   pen_leg_active(Pen::DASH, IBLSCALE(2), MapGfx.TaskColor),
   pen_leg_inactive(Pen::DASH, IBLSCALE(1), MapGfx.TaskColor),
   pen_leg_arrow(Pen::SOLID, IBLSCALE(1), MapGfx.TaskColor),
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
  buffer_render_start();
  
  if (m_layer == 1) {
    draw_task_line(m_location, tp.get_location_remaining());
  }
  if (m_layer == 3) {
    draw_bearing(tp);
  }
  m_index++;
}

void 
RenderTaskPoint::draw_ordered(const OrderedTaskPoint& tp) 
{
  buffer_render_start();

  if (m_layer == 0) {
    draw_oz_background(tp);
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
  
  m_buffer.select(MapGfx.hpBearing);
  draw_great_circle(m_buffer, m_location,
                    tp.get_location_remaining());
}

void 
RenderTaskPoint::draw_target(const TaskPoint &tp) 
{
  if (!do_draw_target(tp)) 
    return;

/*  
  m_map.draw_masked_bitmap_if_visible(m_buffer, MapGfx.hBmpTarget,
                                      tp.get_location_remaining(),
                                      10, 10);
*/
}

void 
RenderTaskPoint::draw_task_line(const GEOPOINT& start, const GEOPOINT& end) 
{
  if (leg_active()) {
    m_buffer.select(pen_leg_active);
  } else {
    m_buffer.select(pen_leg_inactive);
  }
  draw_great_circle(m_buffer, start, end);
  
  // draw small arrow along task direction
  POINT p_p;
  POINT Arrow[3] = { {6,6}, {-6,6}, {0,0} };
  
  POINT p_start;
  m_proj.LonLat2Screen(start, p_start);
  
  POINT p_end;
  m_proj.LonLat2Screen(end, p_end);
  
  const fixed ang = AngleLimit360(atan2(fixed(p_end.x - p_start.x),
                                        fixed(p_start.y - p_end.y)) * fixed_rad_to_deg);
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
  if (!do_draw_isoline(tp)) {
    return;
  }
  AATIsolineSegment seg(tp);
  if (!seg.valid()) {
    return;
  }
  std::vector<POINT> screen; 
  static const fixed fixed_twentieth(1.0 / 20.0);
  
  if (m_proj.DistanceMetersToScreen(seg.parametric(fixed_zero).
                                    distance(seg.parametric(fixed_one)))>2) {
    
    for (fixed t = fixed_zero; t<=fixed_one; t+= fixed_twentieth) {
      GEOPOINT ga = seg.parametric(t);
      POINT sc;
      m_proj.LonLat2Screen(ga, sc);
      screen.push_back(sc);
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
    m_buffer.set_text_color(MapGfx.Colours[m_settings_map.
    iAirspaceColour[1]]);
    // get brush, can be solid or a 1bpp bitmap
    m_buffer.select(MapGfx.hAirspaceBrushes[m_settings_map.
    iAirspaceBrush[1]]);
    */
    // erase where aircraft has been
    m_buffer.white_brush();
    m_buffer.white_pen();
    
    draw_search_point_vector(m_buffer, tp.get_sample_points());
}

void 
RenderTaskPoint::draw_oz_background(const OrderedTaskPoint& tp) 
{
  ozv.set_past(point_past());
  ozv.set_current(point_current());
  ozv.set_background(true);
  tp.CAccept_oz(ozv);
}

void 
RenderTaskPoint::draw_oz_foreground(const OrderedTaskPoint& tp) 
{
  ozv.set_past(point_past());
  ozv.set_current(point_current());
  ozv.set_background(false);
  tp.CAccept_oz(ozv);
}
