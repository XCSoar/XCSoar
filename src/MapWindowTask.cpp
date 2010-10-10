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

#include "MapWindow.hpp"
#include "Protection.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Math/Earth.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "RenderTask.hpp"
#include "RenderTaskPoint.hpp"
#include "RenderObservationZone.hpp"

#include <stdio.h>
#include <math.h>

class RenderTaskPointMap: public RenderTaskPoint
{
protected:
  const MapWindowProjection &projection;
  LabelBlock &label_block;
  BitmapCanvas &bitmap_canvas;

public:
  RenderTaskPointMap(Canvas &_canvas, const MapWindowProjection &_projection,
                     const SETTINGS_MAP &_settings_map,
                     RenderObservationZone &_ozv,
                     const bool draw_bearing,
                     const GeoPoint &location,
                     LabelBlock &_label_block,
                     BitmapCanvas &_bitmap_canvas,
                     const Angle bearing,
                     const bool do_draw_off_track):
    RenderTaskPoint(_canvas, _projection, _settings_map,
                    _ozv, draw_bearing, location),
    projection(_projection),
    label_block(_label_block),
    bitmap_canvas(_bitmap_canvas),
    m_bearing(bearing),
    m_draw_off_track(do_draw_off_track) {};

protected:
  void
  draw_target(const TaskPoint &tp)
  {
    if (!do_draw_target(tp))
      return;

    POINT sc;
    if (m_proj.LonLat2ScreenIfVisible(tp.get_location_remaining(), &sc))
      Graphics::hBmpTarget.draw(m_buffer, bitmap_canvas, sc.x, sc.y);
  }

  void
  draw_off_track(const TaskPoint &tp)
  {
    if (!m_draw_off_track)
      return;

    if (!point_current())
      return;

    GeoVector vec(m_location, tp.get_location_remaining());

    if ((m_bearing - vec.Bearing).as_delta().magnitude_degrees() < fixed(10))
      // insignificant error
      return;

    fixed distance_max =
        min(vec.Distance,
            projection.GetScreenDistanceMeters() * fixed(0.7));

    // too short to bother
    if (distance_max < fixed(5000))
      return;

    GeoPoint start = m_location;
    GeoPoint target = tp.get_location_remaining();

    m_canvas.select(Fonts::Title);
    m_canvas.set_text_color(Color::BLACK);
    m_canvas.background_transparent();

    GeoPoint dloc;
    int ilast = 0;
    for (fixed d = fixed_one / 4; d <= fixed_one; d += fixed_one / 4) {
      FindLatitudeLongitude(start, m_bearing, distance_max * d, &dloc);

      fixed distance0 = Distance(start, dloc);
      fixed distance1 = Distance(dloc, target);
      fixed distance = fixed(distance0 + distance1) / vec.Distance;
      int idist = iround((distance - fixed_one) * 100);

      if ((idist != ilast) && (idist > 0) && (idist < 1000)) {
        TCHAR Buffer[5];
        _stprintf(Buffer, _T("%d"), idist);
        POINT sc = m_proj.LonLat2Screen(dloc);
        RECT brect;
        SIZE tsize = m_canvas.text_size(Buffer);

        brect.left = sc.x - 4;
        brect.right = brect.left + tsize.cx + 4;
        brect.top = sc.y - 4;
        brect.bottom = brect.top + tsize.cy + 4;

        if (label_block.check(brect)) {
          m_canvas.text(sc.x - tsize.cx / 2, sc.y - tsize.cy / 2, Buffer);
          ilast = idist;
        }
      }
    }
  }

private:
  const Angle m_bearing;
  const bool m_draw_off_track;
};

void
MapWindow::DrawTask(Canvas &canvas, const RECT &rc, Canvas &buffer)
{
  if (task == NULL)
    return;

  ProtectedTaskManager::Lease task_manager(*task);
  if (!task_manager->check_task())
    return;

  /* RLD bearing is invalid if GPS not connected and in non-sim mode,
   but we can still draw targets */
  const bool draw_bearing = Basic().gps.Connected;

  RenderObservationZone ozv(canvas, render_projection, SettingsMap());
  RenderTaskPointMap tpv(canvas, render_projection, SettingsMap(),
                         ozv, draw_bearing,
                         Basic().Location,
                         getLabelBlock(), get_bitmap_canvas(),
                         Basic().TrackBearing,
                         !Calculated().Circling);
  RenderTask dv(tpv);
  task_manager->CAccept(dv);
}

#ifdef OLD_TASK // projected track line

void
MapWindow::DrawProjectedTrack(Canvas &canvas) const
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

  GeoPoint start = Basic().Location;
  GeoPoint previous_loc = task->getTargetLocation(task->getActiveIndex() - 1);

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

    GeoPoint p1, p2;
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
    pt[1].y = (long)(max(canvas.get_width(), canvas.get_height()) * -1.2);
    PolygonRotateShift(pt, 2, Orig_Aircraft.x, Orig_Aircraft.y,
		       bearing-DisplayAngle);
  }

  Pen dash_pen(Pen::DASH, IBLSCALE(2), Color(0, 0, 0));
  canvas.select(dash_pen);
  canvas.line(pt[0], pt[1]);
}


#endif
