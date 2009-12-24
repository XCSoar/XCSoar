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

#include "Task/TaskManager.hpp"
#include "Task/Visitors/TaskVisitor.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Task/Visitors/ObservationZoneVisitor.hpp"
#include "Task/TaskPoints/AATIsolineSegment.hpp"

#include "MapWindow.h"
#include "Protection.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Math/Screen.hpp"
#include "Math/Earth.hpp"
#include "Math/Geometry.hpp"
#include <math.h>
#include "MapDrawHelper.hpp"


class DrawObservationZone: 
  public ObservationZoneVisitor, 
  public MapDrawHelper
{
public:
  DrawObservationZone(MapDrawHelper &_draw)
    :MapDrawHelper(_draw),
     pen_boundary_current(Pen::SOLID, IBLSCALE(2), MapGfx.TaskColor),
     pen_boundary_active(Pen::SOLID, IBLSCALE(1), MapGfx.TaskColor),
     pen_boundary_inactive(Pen::SOLID, IBLSCALE(1), Color(127, 127, 127)),
     m_past(false),
     m_current(false),
     m_background(false)
    {};

  bool draw_style(bool is_boundary_active) {
    if (m_background) {
      // this color is used as the black bit
      m_buffer.set_text_color(MapGfx.Colours[m_map.SettingsMap().
                                           iAirspaceColour[AATASK]]);
      // get brush, can be solid or a 1bpp bitmap
      m_buffer.select(MapGfx.hAirspaceBrushes[m_map.SettingsMap().
                                            iAirspaceBrush[AATASK]]);
      m_buffer.white_pen();

      return !m_past;
    } else {
      m_buffer.hollow_brush();
      if (is_boundary_active) {
        if (m_current) {
          m_buffer.select(pen_boundary_current);
        } else {
          m_buffer.select(pen_boundary_active);
        }
      } else {
        m_buffer.select(pen_boundary_inactive); 
      }
      return true;
    }
  }

  void draw_two_lines() {
    m_buffer.two_lines(p_start, p_center, p_end);
  }

  void draw_circle() {
    m_buffer.circle(p_center.x, p_center.y, p_radius);
  }

  void draw_segment(const fixed start_radial, const fixed end_radial) {
    m_buffer.segment(p_center.x, p_center.y, p_radius, m_rc, 
                     start_radial-m_map.GetDisplayAngle(), 
                     end_radial-m_map.GetDisplayAngle());
  }

  void parms_oz(const CylinderZone& oz) {
    buffer_render_start();
    p_radius = m_map.DistanceMetersToScreen(oz.getRadius());
    m_map.LonLat2Screen(oz.get_location(), p_center);
  }

  void parms_sector(const SectorZone& oz) {
    parms_oz(oz);
    m_map.LonLat2Screen(oz.get_SectorStart(), p_start);
    m_map.LonLat2Screen(oz.get_SectorEnd(), p_end);
  }

  void Visit(const FAISectorZone& oz) {
    parms_sector(oz);
    if (draw_style(false)) {
      draw_segment(oz.getStartRadial(), oz.getEndRadial());
    }
    if (draw_style(!m_past)) {
      draw_two_lines();
    }
  }

  void Visit(const SectorZone& oz) {
    parms_sector(oz);
    if (draw_style(!m_past)) {
      draw_segment(oz.getStartRadial(), oz.getEndRadial());
      draw_two_lines();
    }
  }
  void Visit(const LineSectorZone& oz) {
    parms_sector(oz);
    if (draw_style(false)) {
      draw_segment(oz.getStartRadial(), oz.getEndRadial());
    }
    if (draw_style(!m_past)) {
      draw_two_lines();
    }
  }
  void Visit(const CylinderZone& oz) {
    parms_oz(oz);
    if (draw_style(!m_past)) {
      draw_circle();
    }
  }
  void set_past(bool set) {
    m_past = set;
  }
  void set_current(bool set) {
    m_current = set;
  }
  void set_background(bool set) {
    m_background = set;
  }

private:
  const Pen pen_boundary_current;
  const Pen pen_boundary_active;
  const Pen pen_boundary_inactive;
  POINT p_center, p_start, p_end;
  unsigned p_radius;
  bool m_past;
  bool m_current;
  bool m_background;
};


class DrawTaskPoint:
  public TaskPointVisitor,
  public MapDrawHelper
{
public:
  DrawTaskPoint(MapDrawHelper &_helper, 
                const bool draw_bearing)
    :MapDrawHelper(_helper),
     m_draw_bearing(draw_bearing),
     pen_leg_active(Pen::DASH, IBLSCALE(2), MapGfx.TaskColor),
     pen_leg_inactive(Pen::DASH, IBLSCALE(1), MapGfx.TaskColor),
     pen_leg_arrow(Pen::SOLID, IBLSCALE(1), MapGfx.TaskColor),
     pen_isoline(Pen::SOLID, IBLSCALE(2), Color(0,0,255)), 
     m_index(0),
     ozv(*this),
     m_active_index(0),
     m_layer(0)
    {}

  void set_layer(unsigned set) {
    m_layer = set;
    m_index = 0;
  }

  void Visit(const UnorderedTaskPoint& tp) {
    buffer_render_start();

    if (m_layer == 1) {
      POINT loc;
      m_map.LonLat2Screen(tp.get_location_remaining(), loc);
      draw_task_line(m_map.GetOrigAircraft(), loc);
    }
    if (m_layer == 3) {
      draw_bearing(tp);
    }
    m_index++;
  }

  void draw_ordered(const OrderedTaskPoint& tp) {
    buffer_render_start();

    if (m_layer == 0) {
      draw_oz_background(tp);
      draw_samples(tp);
    }

    if (m_layer == 1) {
      POINT loc;
      m_map.LonLat2Screen(tp.get_location_remaining(), loc);    
      if (m_index>0) {
        draw_task_line(m_last_point, loc);
      }
      m_last_point = loc;
    }

    if (m_layer == 2) {
      draw_oz_foreground(tp);
    }
  }

  void Visit(const StartPoint& tp) {
    m_index = 0;
    draw_ordered(tp);
    if (m_layer == 3) {
      draw_bearing(tp);
      draw_target(tp);
    }
  }
  void Visit(const FinishPoint& tp) {
    m_index++;
    draw_ordered(tp);
    if (m_layer == 3) {
      draw_bearing(tp);
      draw_target(tp);
    }
  }
  void Visit(const AATPoint& tp) {
    m_index++;

    draw_ordered(tp);
    if (m_layer == 3) {
      draw_isoline(tp);
      draw_bearing(tp);
      draw_target(tp);
    }
  }
  void Visit(const ASTPoint& tp) {
    m_index++;

    draw_ordered(tp);
    if (m_layer == 3) {
      draw_bearing(tp);
      draw_target(tp);
    }
  }

  void set_active_index(unsigned active_index) {
    m_active_index = active_index;
  }

private:
  bool leg_active() {
    return (m_index+1>m_active_index);
  }
  bool point_past() {
    return (m_index<m_active_index);
  }
  bool point_current() {
    return (m_index==m_active_index);
  }

  bool do_draw_samples(const TaskPoint& tp) {
    return point_current() || point_past();
  }

  bool do_draw_bearing(const TaskPoint &tp) {
    return m_draw_bearing && point_current();
  }

  bool do_draw_target(const TaskPoint &tp) {
    if (!tp.has_target()) {
      return false;
    }
    return (point_current()
            || m_map.SettingsMap().EnablePan 
            || m_map.SettingsMap().TargetPan);
  }

  bool do_draw_isoline(const TaskPoint &tp) {
    return do_draw_target(tp);
  }

  void draw_bearing(const TaskPoint &tp) {
    if (!do_draw_bearing(tp)) 
      return;

    m_map.DrawGreatCircle(m_buffer, 
                          m_map.Basic().Location, 
                          tp.get_location_remaining());
  }

  void draw_target(const TaskPoint &tp) {
    if (!do_draw_target(tp)) 
      return;

    m_map.draw_masked_bitmap_if_visible(m_buffer, MapGfx.hBmpTarget,
                                        tp.get_location_remaining(),
                                        10, 10);
  }

  void draw_task_line(const POINT& start, const POINT& end) {
    if (leg_active()) {
      m_buffer.select(pen_leg_active);
    } else {
      m_buffer.select(pen_leg_inactive);
    }
    m_buffer.line(start, end);

    // draw small arrow along task direction
    POINT p_p;
    POINT Arrow[3] = { {6,6}, {-6,6}, {0,0} };

    const double ang = AngleLimit360(atan2(end.x-start.x,start.y-end.y)*180/3.141592);
    ScreenClosestPoint(start, end, m_map.GetOrigScreen(), &p_p, IBLSCALE(25));
    PolygonRotateShift(Arrow, 2, p_p.x, p_p.y, ang);
    Arrow[2] = Arrow[1];
    Arrow[1] = p_p;

    m_buffer.select(pen_leg_arrow);
    m_buffer.polyline(Arrow, 3);
  }

  void draw_isoline(const AATPoint& tp) {
    if (!do_draw_isoline(tp)) {
      return;
    }
    AATIsolineSegment seg(tp);
    if (!seg.valid()) {
      return;
    }
    std::vector<POINT> screen; 
    static const fixed fixed_twentieth(1.0 / 20.0);

    if (m_map.DistanceMetersToScreen(seg.parametric(fixed_zero).
                                     distance(seg.parametric(fixed_one)))>2) {

      for (fixed t = fixed_zero; t<=fixed_one; t+= fixed_twentieth) {
        GEOPOINT ga = seg.parametric(t);
        POINT sc;
        m_map.LonLat2Screen(ga, sc);
        screen.push_back(sc);
      }
      if (screen.size()>=2) {
        m_buffer.select(pen_isoline);
        m_buffer.polyline(&screen[0], screen.size());
      }
    }
  }

  void draw_samples(const OrderedTaskPoint& tp) {
    if (!do_draw_samples(tp)) {
      return;
    }
    /*
    m_buffer.set_text_color(MapGfx.Colours[m_map.SettingsMap().
                                           iAirspaceColour[1]]);
    // get brush, can be solid or a 1bpp bitmap
    m_buffer.select(MapGfx.hAirspaceBrushes[m_map.SettingsMap().
                                            iAirspaceBrush[1]]);
    */
    // erase where aircraft has been
    m_buffer.white_brush();
    m_buffer.white_pen();
    
    draw_search_point_vector(m_buffer, tp.get_sample_points());
  }

  void draw_oz_background(const OrderedTaskPoint& tp) {
    ozv.set_past(point_past());
    ozv.set_current(point_current());
    ozv.set_background(true);
    tp.Accept_oz(ozv);
  }

  void draw_oz_foreground(const OrderedTaskPoint& tp) {
    ozv.set_past(point_past());
    ozv.set_current(point_current());
    ozv.set_background(false);
    tp.Accept_oz(ozv);
  }

  const bool& m_draw_bearing;
  const Pen pen_leg_active;
  const Pen pen_leg_inactive;
  const Pen pen_leg_arrow;
  const Pen pen_isoline;
  POINT m_last_point;
  unsigned m_index;
  DrawObservationZone ozv;
  unsigned m_active_index;
  unsigned m_layer;
};



class DrawTaskVisitor: 
  public TaskVisitor,
  public MapDrawHelper
{
public:
  DrawTaskVisitor(MapDrawHelper &_helper,
                  bool draw_bearing):
    MapDrawHelper(_helper),
    tpv(*this, draw_bearing)
    {};

  void draw_layers(const AbstractTask& task) {
    for (unsigned i=0; i<4; i++) {
      tpv.set_layer(i);
      task.Accept(tpv);
    }
  }
  void Visit(const AbortTask& task) {
    tpv.set_active_index(task.getActiveIndex());
    draw_layers(task);
  }
  void Visit(const OrderedTask& task) {
    tpv.set_active_index(task.getActiveIndex());
    draw_layers(task);
  }
  void Visit(const GotoTask& task) {
    tpv.set_active_index(0);
    draw_layers(task);
  }
private:
  DrawTaskPoint tpv;
};

#include "RasterTerrain.h"

void
MapWindow::DrawTask(Canvas &canvas, const RECT rc, Canvas &buffer)
{
  if (task == NULL)
    return;

  /* RLD bearing is invalid if GPS not connected and in non-sim mode,
   but we can still draw targets */
  const bool draw_bearing = Basic().Connected;

  terrain->Lock(); 
  {
    MapDrawHelper helper(canvas, buffer, stencil_canvas, *this, rc);
    DrawTaskVisitor dv(helper, draw_bearing);
    task->Accept(dv); 
  }
  terrain->Unlock();
}


/////////////////////////////////////////////////////////////////////////////



#ifdef OLD_TASK

void
MapWindow::DrawOffTrackIndicator(Canvas &canvas)
{
  if (task == NULL || !task->Valid() || task->getActiveIndex() <= 0)
    return;

  if (fabs(Basic().TrackBearing-Calculated().WaypointBearing)<10) {
    // insignificant error
    return;
  }
  if (Calculated().Circling || task->TaskIsTemporary() ||
      SettingsMap().TargetPan) {
    // don't display in various modes
    return;
  }

  double distance_max = min(Calculated().WaypointDistance,
			    GetScreenDistanceMeters()*0.7);
  if (distance_max < 5000.0) {
    // too short to bother
    return;
  }

  GEOPOINT start = Basic().Location;
  GEOPOINT target = task->getTargetLocation();

  canvas.select(TitleWindowFont);
  canvas.set_text_color(Color(0x0, 0x0, 0x0));

  GEOPOINT dloc;
  int ilast = 0;
  for (double d=0.25; d<=1.0; d+= 0.25) {
    FindLatitudeLongitude(start,
			  Basic().TrackBearing,
			  distance_max*d,
			  &dloc);

    double distance0 = Distance(start, dloc);
    double distance1 = Distance(dloc, target);
    double distance = (distance0+distance1)/Calculated().WaypointDistance;
    int idist = iround((distance-1.0)*100);

    if ((idist != ilast) && (idist>0) && (idist<1000)) {

      TCHAR Buffer[5];
      _stprintf(Buffer, TEXT("%d"), idist);
      POINT sc;
      RECT brect;
      LonLat2Screen(dloc, sc);
      SIZE tsize = canvas.text_size(Buffer);

      brect.left = sc.x-4;
      brect.right = brect.left+tsize.cx+4;
      brect.top = sc.y-4;
      brect.bottom = brect.top+tsize.cy+4;

      if (label_block.check(brect)) {
        canvas.text(sc.x - tsize.cx / 2, sc.y - tsize.cy / 2, Buffer);
	ilast = idist;
      }
    }
  }
}

void
MapWindow::DrawProjectedTrack(Canvas &canvas)
{
  if (task == NULL || !task->Valid() || !task->getSettings().AATEnabled ||
      task->getActiveIndex() ==0)
    return;

  if (Calculated().Circling || task->TaskIsTemporary()) {
    // don't display in various modes
    return;
  }

  // TODO feature: maybe have this work even if no task?
  // TODO feature: draw this also when in target pan mode

  GEOPOINT start = Basic().Location;
  GEOPOINT previous_loc = task->getTargetLocation(task->getActiveIndex() - 1);

  double distance_from_previous, bearing;
  DistanceBearing(previous_loc, start,
		  &distance_from_previous,
		  &bearing);

  if (distance_from_previous < 100.0) {
    bearing = Basic().TrackBearing;
    // too short to have valid data
  }
  POINT pt[2] = {{0,-75},{0,-400}};
  if (SettingsMap().TargetPan) {
    double screen_range = GetScreenDistanceMeters();
    double f_low = 0.4;
    double f_high = 1.5;
    screen_range = max(screen_range, Calculated().WaypointDistance);

    GEOPOINT p1, p2;
    FindLatitudeLongitude(start,
			  bearing, f_low*screen_range,
			  &p1);
    FindLatitudeLongitude(start,
			  bearing, f_high*screen_range,
			  &p2);
    LonLat2Screen(p1, pt[0]);
    LonLat2Screen(p2, pt[1]);
  } else if (fabs(bearing-Calculated().WaypointBearing)<10) {
    // too small an error to bother
    return;
  } else {
    pt[1].y = (long)(-max(MapRectBig.right-MapRectBig.left,
			  MapRectBig.bottom-MapRectBig.top)*1.2);
    PolygonRotateShift(pt, 2, Orig_Aircraft.x, Orig_Aircraft.y,
		       bearing-DisplayAngle);
  }

  Pen dash_pen(Pen::DASH, IBLSCALE(2), Color(0, 0, 0));
  canvas.select(dash_pen);
  canvas.line(pt[0], pt[1]);
}


#endif
