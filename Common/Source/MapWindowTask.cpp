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
    for (double t = 0.0; t<=1.0; t+= 1.0/20) {
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

  void draw_samples(const OrderedTaskPoint& tp) {
    if (!do_draw_samples(tp)) {
      return;
    }
    m_buffer.set_text_color(MapGfx.Colours[m_map.SettingsMap().
                                           iAirspaceColour[1]]);
    // get brush, can be solid or a 1bpp bitmap
    m_buffer.select(MapGfx.hAirspaceBrushes[m_map.SettingsMap().
                                            iAirspaceBrush[1]]);
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

  /* RLD bearing is invalid if GPS not connected and in non-sim mode,
   but we can still draw targets */
  const bool draw_bearing = Basic().Connected;

  terrain->Lock(); 
  {
    MapDrawHelper helper(canvas, buffer, *this, rc);
    DrawTaskVisitor dv(helper, draw_bearing);
    task->Accept(dv); 
  }
  terrain->Unlock();
}


/////////////////////////////////////////////////////////////////////////////



#ifdef OLD_TASK

class DrawTaskVisitor:
  public AbsoluteTaskPointVisitor,
  public AbsoluteTaskLegVisitor
{
  const WayPointList &way_points;

public:
  DrawTaskVisitor(MapWindow &_map_window,
		  Canvas &_canvas,
		  POINT &_orig,
		  TaskScreen_t &_task_screen,
		  StartScreen_t &_start_screen,
                  unsigned _activeIndex,
                  const WayPointList &_way_points):
    map_window(&_map_window),
    canvas(&_canvas),
    orig(_orig),
    task_screen(&_task_screen),
    start_screen(&_start_screen),
    pent1(Pen::SOLID, IBLSCALE(1), MapGfx.TaskColor),
    penb2(Pen::SOLID, IBLSCALE(2), Color(0,0,255)),
    dash_pen3(Pen::DASH, IBLSCALE(3), MapGfx.TaskColor),
    dash_pen5(Pen::DASH, IBLSCALE(5), MapGfx.TaskColor),
    dash_pen2(Pen::DASH, IBLSCALE(2), Color(127, 127, 127)),
    activeIndex(_activeIndex),
    way_points(_way_points)
  {
  }

  void
  visit_start_point(START_POINT &point, const unsigned index)
  {
    DrawStartSector((*start_screen)[index].SectorStart,
                    (*start_screen)[index].SectorEnd, point.Index);
  };

  void
  visit_task_point_start(TASK_POINT &point, const unsigned index)
  {
    DrawStartSector((*task_screen)[index].SectorStart,
                    (*task_screen)[index].SectorEnd, point.Index);
  };

  void
  visit_task_point_intermediate_aat(TASK_POINT &point, const unsigned i)
  {
    // JMW added iso lines
    if ((i==activeIndex)
        || (map_window->SettingsMap().TargetPan
            && ((int)i==map_window->SettingsMap().TargetPanIndex))) {
      // JMW 20080616 flash arc line if very close to target
      static bool flip = false;

      if (map_window->Calculated().WaypointDistance<200.0) { // JMW hardcoded AATCloseDistance
        flip = !flip;
      } else {
        flip = true;
      }

      if (flip) {
        for (int j=0; j<MAXISOLINES-1; j++) {
          if (point.IsoLine_valid[j] && point.IsoLine_valid[j+1]) {
            canvas->select(penb2);
            canvas->line((*task_screen)[i].IsoLine_Screen[j],
                         (*task_screen)[i].IsoLine_Screen[j + 1]);
          }
        }
      }
    }
  }

  void
  visit_task_point_intermediate_non_aat(TASK_POINT &point, const unsigned i)
  {
    const POINT &wp = way_points.get_calc(point.Index).Screen;

    canvas->select(dash_pen2);
    canvas->two_lines((*task_screen)[i].SectorStart,
		      wp,
		     (*task_screen)[i].SectorEnd);

    canvas->hollow_brush();
    canvas->black_pen();

    if (_task->getSettings().SectorType== 0) {
      unsigned tmp = map_window->DistanceMetersToScreen(_task->getSettings().SectorRadius);
      canvas->circle(wp.x, wp.y, tmp);
    } else if (_task->getSettings().SectorType==1) {
      unsigned tmp = map_window->DistanceMetersToScreen(_task->getSettings().SectorRadius);
      canvas->segment(wp.x, wp.y, tmp,
		      map_window->GetMapRect(),
		      point.AATStartRadial-map_window->GetDisplayAngle(),
		      point.AATFinishRadial-map_window->GetDisplayAngle());
    } else if(_task->getSettings().SectorType== 2) {
      unsigned tmp;
      tmp = map_window->DistanceMetersToScreen(500);
      canvas->circle(wp.x, wp.y, tmp);

      tmp = map_window->DistanceMetersToScreen(10000);
      canvas->segment(wp.x, wp.y, tmp, map_window->GetMapRect(),
		      point.AATStartRadial-map_window->GetDisplayAngle(),
		      point.AATFinishRadial-map_window->GetDisplayAngle());
    }
  }


  void
  visit_task_point_intermediate(TASK_POINT &point, const unsigned index)
  {
    if(_task->getSettings().AATEnabled) {
      visit_task_point_intermediate_aat(point, index);
    } else {
      visit_task_point_intermediate_non_aat(point, index);
    }
  };

  void
  visit_task_point_final(TASK_POINT &point, const unsigned index)
  {
    if (activeIndex > 1) {
      // only draw finish line when past the first waypoint
      const POINT &wp = way_points.get_calc(point.Index).Screen;

      if(_task->getSettings().FinishType != FINISH_CIRCLE) {
        canvas->select(dash_pen5);
        canvas->two_lines((*task_screen)[index].SectorStart, wp,
            (*task_screen)[index].SectorEnd);
        canvas->select(MapGfx.hpStartFinishThin);
        canvas->two_lines((*task_screen)[index].SectorStart, wp,
            (*task_screen)[index].SectorEnd);
      } else {
        unsigned tmp = map_window->
            DistanceMetersToScreen(_task->getSettings().FinishRadius);

        canvas->hollow_brush();
        canvas->select(MapGfx.hpStartFinishThick);
        canvas->circle(wp.x, wp.y, tmp);
        canvas->select(MapGfx.hpStartFinishThin);
        canvas->circle(wp.x, wp.y, tmp);
      }
    }
  };

  void
  visit_leg_multistart(START_POINT &start, const unsigned index0,
                       TASK_POINT &point)
  {
    // nothing to draw
  };

  void
  visit_leg_intermediate(TASK_POINT &point0, const unsigned index0,
                         TASK_POINT &point1, const unsigned index1)
  {
    visit_leg_final(point0, index0, point1, index1);
  };

  void
  visit_leg_final(TASK_POINT &point0, const unsigned index0,
                  TASK_POINT &point1, const unsigned index1)
  {
    bool is_first = (point0.Index < point1.Index);
    int imin = min(point0.Index,point1.Index);
    int imax = max(point0.Index,point1.Index);
    // JMW AAT!
    double bearing = point0.OutBound;
    POINT sct1, sct2;

    canvas->select(dash_pen3);

    if (_task->getSettings().AATEnabled && !map_window->SettingsMap().TargetPan) {
      map_window->LonLat2Screen(point0.AATTargetLocation, sct1);
      map_window->LonLat2Screen(point1.AATTargetLocation, sct2);
      bearing = Bearing(point0.AATTargetLocation, point1.AATTargetLocation);

      // draw nominal track line
      canvas->line(way_points.get_calc(imin).Screen,
                   way_points.get_calc(imax).Screen);
    } else {
      sct1 = way_points.get_calc(point0.Index).Screen;
      sct2 = way_points.get_calc(point1.Index).Screen;
    }

    if (is_first) {
      canvas->line(sct1, sct2);
    } else {
      canvas->line(sct2, sct1);
    }

    // draw small arrow along task direction
    POINT p_p;
    POINT Arrow[3] = { {6,6}, {-6,6}, {0,0} };
    ScreenClosestPoint(sct1, sct2, orig, &p_p, IBLSCALE(25));
    PolygonRotateShift(Arrow, 2, p_p.x, p_p.y,
		       bearing-map_window->GetDisplayAngle());
    Arrow[2] = Arrow[1];
    Arrow[1] = p_p;

    canvas->select(pent1);
    canvas->polyline(Arrow, 3);
  };

private:
  MapWindow *map_window;
  Canvas* canvas;
  const POINT orig;
  const TaskScreen_t *task_screen;
  const StartScreen_t *start_screen;
  const Pen pent1;
  const Pen penb2;
  const Pen dash_pen3;
  const Pen dash_pen5;
  const Pen dash_pen2;
  unsigned activeIndex;

  void
  DrawStartSector(const POINT &Start,
                  const POINT &End, const unsigned Index)
  {
    if (activeIndex>=2) {
      // don't draw if on second leg or beyond
      return;
    }

    const WPCALC &wpcalc = way_points.get_calc(Index);
    if(_task->getSettings().StartType != START_CIRCLE) {
      canvas->select(MapGfx.hpStartFinishThick);
      canvas->line(wpcalc.Screen, Start);
      canvas->line(wpcalc.Screen, End);
      canvas->select(MapGfx.hpStartFinishThin);
      canvas->line(wpcalc.Screen, Start);
      canvas->line(wpcalc.Screen, End);
    } else {
      unsigned tmp = map_window->DistanceMetersToScreen(_task->getSettings().StartRadius);
      canvas->hollow_brush();
      canvas->select(MapGfx.hpStartFinishThick);
      canvas->circle(wpcalc.Screen.x, wpcalc.Screen.y, tmp);
      canvas->select(MapGfx.hpStartFinishThin);
      canvas->circle(wpcalc.Screen.x, wpcalc.Screen.y, tmp);
    }
  };
};

void
MapWindow::DrawTask(Canvas &canvas, RECT rc)
{
  if (way_points == NULL || task == NULL)
    return;

  DrawTaskVisitor dv(*this, canvas, Orig_Aircraft, task_screen, task_start_screen,
                     task->getActiveIndex(), *way_points);
  task->scan_leg_forward(dv, false); // read lock
  task->scan_point_forward(dv, false); // read lock
}

class DrawTaskAATVisitor:
  public AbsoluteTaskPointVisitor
{
  const WayPointList &way_points;

public:
  DrawTaskAATVisitor(Canvas &_canvas,
                     const RECT _rc,
                     Canvas &_buffer,
                     const unsigned _activeIndex,
                     TaskScreen_t &_task_screen,
                     MapWindowProjection &_map,
                     const SETTINGS_MAP &_settings,
                     const WayPointList &_way_points
    ):
    canvas(_canvas), rc(_rc), buffer(_buffer), activeIndex(_activeIndex),
    task_screen(_task_screen), map(_map), settings(_settings),
    way_points(_way_points)
  {
    whitecolor = Color(0xff,0xff, 0xff);
    buffer.set_text_color(whitecolor);
    buffer.white_pen();
    buffer.white_brush();
    buffer.rectangle(rc.left, rc.top, rc.right, rc.bottom);
  }

  void
  visit_task_point_start(TASK_POINT &point, const unsigned index)
  {
  };

  void
  visit_task_point_intermediate(TASK_POINT &point, const unsigned i)
  {
    const WPCALC &wpcalc = way_points.get_calc(point.Index);
    unsigned tmp;

    if (point.AATType == AAT_CIRCLE) {
      tmp = map.DistanceMetersToScreen(point.AATCircleRadius);

      // this color is used as the black bit
      buffer.set_text_color(MapGfx.Colours[settings.iAirspaceColour[AATASK]]);

      // this color is the transparent bit
      buffer.set_background_color(whitecolor);

      if (i<activeIndex) {
        buffer.hollow_brush();
      } else {
        buffer.select(MapGfx.hAirspaceBrushes[settings.iAirspaceBrush[AATASK]]);
      }
      buffer.black_pen();

      buffer.circle(wpcalc.Screen.x, wpcalc.Screen.y, tmp);
    } else {

      // this color is used as the black bit
      buffer.set_text_color(MapGfx.Colours[settings.iAirspaceColour[AATASK]]);

      // this color is the transparent bit
      buffer.set_background_color(whitecolor);

      if (i<activeIndex) {
        buffer.hollow_brush();
      } else {
        buffer.select(MapGfx.hAirspaceBrushes[settings.iAirspaceBrush[AATASK]]);
      }
      buffer.black_pen();

      tmp = map.DistanceMetersToScreen(point.AATSectorRadius);

      buffer.segment(wpcalc.Screen.x,
                     wpcalc.Screen.y, tmp, rc,
                     point.AATStartRadial-map.GetDisplayAngle(),
                     point.AATFinishRadial-map.GetDisplayAngle());

      buffer.two_lines(task_screen[i].AATStart, wpcalc.Screen,
                       task_screen[i].AATFinish);
    }
  };

  void
  visit_task_point_final(TASK_POINT &point, const unsigned index)
  {
  };

private:
  MapWindowProjection &map;
  unsigned activeIndex;
  Canvas &canvas;
  Canvas &buffer;
  RECT rc;
  Color whitecolor;
  TaskScreen_t &task_screen;
  const SETTINGS_MAP &settings;
};

/**
 * Draw the AAT areas to the buffer and copy the buffer to the drawing canvas
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 * @param buffer The drawing buffer
 */
void
MapWindow::DrawTaskAAT(Canvas &canvas, const RECT rc, Canvas &buffer)
{
  if (way_points == NULL || task == NULL)
    return;

  if (task->getSettings().AATEnabled) {
    DrawTaskAATVisitor dv(canvas, rc, buffer, task->getActiveIndex(),
                          task_screen, *this, SettingsMap(), *way_points);
    task->scan_point_forward(dv, false); // read lock
    // TODO, reverse
    canvas.copy_transparent_white(buffer, rc);
  }
}


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

class ScreenPositionsTaskVisitor:
  public AbsoluteTaskPointVisitor {
public:
  ScreenPositionsTaskVisitor(MapWindow& _map,
			     TaskScreen_t &_task_screen,
			     StartScreen_t &_start_screen,
                             unsigned _activeIndex):
    map(&_map),
    task_screen(&_task_screen),
    start_screen(&_start_screen),
    activeIndex(_activeIndex)
  {}

  void
  visit_start_point(START_POINT &point, const unsigned i)
  {
    map->LonLat2Screen(point.SectorEnd,
		       (*task_screen)[i].SectorEnd);
    map->LonLat2Screen(point.SectorStart,
		       (*task_screen)[i].SectorStart);

  };

  void
  visit_task_point_start(TASK_POINT &point, const unsigned i)
  {
    if (_task->getSettings().AATEnabled) {
      map->LonLat2Screen(point.AATTargetLocation, (*task_screen)[i].Target);
    }
    map->LonLat2Screen(point.SectorEnd,
		       (*task_screen)[i].SectorEnd);
    map->LonLat2Screen(point.SectorStart,
		       (*task_screen)[i].SectorStart);
    if(_task->getSettings().AATEnabled && (point.AATType == AAT_SECTOR)) {
      map->LonLat2Screen(point.AATStart,
			 (*task_screen)[i].AATStart);
      map->LonLat2Screen(point.AATFinish,
			 (*task_screen)[i].AATFinish);
    }

  };

  void
  visit_task_point_intermediate(TASK_POINT &point, const unsigned i)
  {
    visit_task_point_start(point, i);
    if (_task->getSettings().AATEnabled
        && ((i==activeIndex)
            || (map->SettingsMap().TargetPan
                && ((int)i==map->SettingsMap().TargetPanIndex)))) {

      for (int j=0; j<MAXISOLINES; j++) {
        if (point.IsoLine_valid[j]) {
          map->LonLat2Screen(point.IsoLine_Location[j],
              (*task_screen)[i].IsoLine_Screen[j]);
        }
      }
    }
  };

  void
  visit_task_point_final(TASK_POINT &point, const unsigned i)
  {
    visit_task_point_start(point, i);
  };

private:
  MapWindow* map;
  TaskScreen_t *task_screen;
  StartScreen_t *start_screen;
  unsigned activeIndex;
};

void
MapWindow::CalculateScreenPositionsTask()
{
  if (task == NULL)
    return;

  ScreenPositionsTaskVisitor sv(*this, task_screen, task_start_screen,
                                task->getActiveIndex());
  task->scan_point_forward(sv, false); // read lock
}

#endif
