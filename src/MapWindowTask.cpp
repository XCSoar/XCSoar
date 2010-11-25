/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "MapWindow.hpp"
#include "Protection.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Icon.hpp"
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

public:
  RenderTaskPointMap(Canvas &_canvas, const MapWindowProjection &_projection,
                     const SETTINGS_MAP &_settings_map,
                     RenderObservationZone &_ozv,
                     const bool draw_bearing,
                     const GeoPoint &location):
    RenderTaskPoint(_canvas, _projection, _settings_map,
                    _ozv, draw_bearing, location),
    projection(_projection)
    {};

protected:
  void
  draw_target(const TaskPoint &tp)
  {
    if (!do_draw_target(tp))
      return;

    RasterPoint sc;
    if (m_proj.GeoToScreenIfVisible(tp.get_location_remaining(), sc))
      Graphics::hBmpTarget.draw(m_buffer, sc.x, sc.y);
  }
};


void
MapWindow::DrawTask(Canvas &canvas)
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
                         Basic().Location);
  RenderTask dv(tpv);
  task_manager->CAccept(dv);
}


void
MapWindow::DrawTaskOffTrackIndicator(Canvas &canvas)
{
  if (Calculated().Circling 
      || !Basic().gps.Connected
      || !SettingsMap().EnableDetourCostMarker
      || (task == NULL)) 
    return;

  ProtectedTaskManager::Lease task_manager(*task);
  if (!task_manager->check_task())
    return;

  const TaskPoint* tp = task_manager->getActiveTaskPoint();
  if (!tp) 
    return;

  GeoPoint target = tp->get_location_remaining();
  GeoVector vec(Basic().Location, target);

  if ((Basic().TrackBearing - vec.Bearing).as_delta().magnitude_degrees() < fixed(10))
    // insignificant error
    return;

  fixed distance_max =
    min(vec.Distance, render_projection.GetScreenDistanceMeters() * fixed(0.7));

  // too short to bother
  if (distance_max < fixed(5000))
    return;

  GeoPoint start = Basic().Location;
  
  canvas.select(Fonts::Title);
  canvas.set_text_color(Color::BLACK);
  canvas.background_transparent();
  
  GeoPoint dloc;
  int ilast = 0;
  for (fixed d = fixed_one / 4; d <= fixed_one; d += fixed_one / 4) {
    dloc = FindLatitudeLongitude(start, Basic().TrackBearing, distance_max * d);
    
    fixed distance0 = Distance(start, dloc);
    fixed distance1 = Distance(dloc, target);
    fixed distance = fixed(distance0 + distance1) / vec.Distance;
    int idist = iround((distance - fixed_one) * 100);
    
    if ((idist != ilast) && (idist > 0) && (idist < 1000)) {
      TCHAR Buffer[5];
      _stprintf(Buffer, _T("%d"), idist);
      RasterPoint sc = render_projection.GeoToScreen(dloc);
      RECT brect;
      SIZE tsize = canvas.text_size(Buffer);
      
      brect.left = sc.x - 4;
      brect.right = brect.left + tsize.cx + 4;
      brect.top = sc.y - 4;
      brect.bottom = brect.top + tsize.cy + 4;
      
      if (label_block.check(brect)) {
        canvas.text(sc.x - tsize.cx / 2, sc.y - tsize.cy / 2, Buffer);
        ilast = idist;
      }
    }
  }
}
