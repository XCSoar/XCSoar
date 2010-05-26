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

#include "TaskClientUI.hpp"
#include "MapWindow.hpp"
#include "Protection.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Math/Earth.hpp"
#include <math.h>

#include "RenderTask.hpp"
#include "RenderTaskPoint.hpp"
#include "RenderObservationZone.hpp"

#include <stdio.h>

class RenderTaskPointMap:
  public RenderTaskPoint
{
public:
  RenderTaskPointMap(MapDrawHelper &_helper, 
                     RenderObservationZone &_ozv,
                     const bool draw_bearing,
                     const GEOPOINT location,
                     MapWindow& map,
                     const Angle bearing,
                     const bool do_draw_off_track):
    RenderTaskPoint(_helper, _ozv, draw_bearing, location),
    m_map(map),
    m_bearing(bearing),
    m_draw_off_track(do_draw_off_track) {};

protected:
  void draw_target(const TaskPoint &tp) 
    {
      if (!do_draw_target(tp)) 
        return;
      
      m_map.draw_masked_bitmap_if_visible(m_buffer, MapGfx.hBmpTarget,
                                          tp.get_location_remaining(),
                                          10, 10);
    }
  void draw_off_track(const TaskPoint &tp) 
    {
      if (!m_draw_off_track) 
        return;
      if (!point_current())
        return;
      
      GeoVector vec(m_location, tp.get_location_remaining());

      if ((m_bearing - vec.Bearing).as_delta().magnitude_degrees() < fixed(10)) {
        // insignificant error
        return;
      }

      double distance_max = min(vec.Distance,
                                m_map.GetScreenDistanceMeters() * fixed(0.7));
      if (distance_max < 5000.0) {
        // too short to bother
        return;
      }

      GEOPOINT start = m_location;
      GEOPOINT target = tp.get_location_remaining();

      m_canvas.select(TitleWindowFont);
      m_canvas.set_text_color(Color(0x0, 0x0, 0x0));

      GEOPOINT dloc;
      int ilast = 0;
      for (double d=0.25; d<=1.0; d+= 0.25) {
        FindLatitudeLongitude(start,
                              m_bearing,
                              (fixed)(distance_max*d),
                              &dloc);

        double distance0 = Distance(start, dloc);
        double distance1 = Distance(dloc, target);
        double distance = fixed(distance0 + distance1) / vec.Distance;
        int idist = iround((distance-1.0)*100);

        if ((idist != ilast) && (idist>0) && (idist<1000)) {

          TCHAR Buffer[5];
          _stprintf(Buffer, TEXT("%d"), idist);
          POINT sc;
          RECT brect;
          m_map.LonLat2Screen(dloc, sc);
          SIZE tsize = m_canvas.text_size(Buffer);

          brect.left = sc.x-4;
          brect.right = brect.left+tsize.cx+4;
          brect.top = sc.y-4;
          brect.bottom = brect.top+tsize.cy+4;
          
          if (m_map.getLabelBlock().check(brect)) {
            m_canvas.text(sc.x - tsize.cx / 2, sc.y - tsize.cy / 2, Buffer);
            ilast = idist;
          }
        }
      }
    }

private:
  MapWindow& m_map;
  const Angle m_bearing;
  const bool m_draw_off_track;
};

void
MapWindow::DrawTask(Canvas &canvas, const RECT rc, Canvas &buffer)
{
  if (task == NULL)
    return;

  /* RLD bearing is invalid if GPS not connected and in non-sim mode,
   but we can still draw targets */
  const bool draw_bearing = Basic().gps.Connected;

  {
    MapDrawHelper helper(canvas, buffer, stencil_canvas, *this, rc,
                         SettingsMap());
    RenderObservationZone ozv(helper);
    RenderTaskPointMap tpv(helper, ozv, draw_bearing,
                           Basic().Location, *this, 
                           Basic().TrackBearing,
                           !Calculated().Circling);
    RenderTask dv(tpv);
    task->CAccept(dv); 
  }
}

#ifdef OLD_TASK // projected track line

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
