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

#include "MapWindow.hpp"
#include "Math/Earth.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Graphics.hpp"
#include "Computer/GlideComputer.hpp"

#include <algorithm>

using std::min;
using std::max;

/**
 * This function returns the corresponding SnailTrail
 * color array index to the input
 * @param cv Input value between -1.0 and 1.0
 * @return SnailTrail color array index
 */
gcc_const
static int
GetSnailColorIndex(fixed cv)
{
  return max((short)0, min((short)(NUMSNAILCOLORS - 1),
                           (short)((cv + fixed_one) / 2 * NUMSNAILCOLORS)));
}

void
MapWindow::RenderTrail(Canvas &canvas, const RasterPoint aircraft_pos) const
{
  unsigned min_time = max(0, (int)Basic().time - 600);
  DrawTrail(canvas, aircraft_pos, min_time);
}

void
MapWindow::DrawTrail(Canvas &canvas, const RasterPoint aircraft_pos,
                     unsigned min_time, bool enable_traildrift) const
{
  if (SettingsMap().trail_length == TRAIL_OFF || glide_computer == NULL)
    return;

  const WindowProjection &projection = render_projection;
  const GeoBounds bounds = projection.GetScreenBounds().scale(fixed_four);

  TracePointVector trace;
  glide_computer->LockedCopyTraceTo(trace, min_time,
                                    projection.GetGeoScreenCenter(),
                                    projection.DistancePixelsToMeters(3));
  if (trace.empty())
    return;

  if (!Calculated().wind_available)
    enable_traildrift = false;

  GeoPoint traildrift;
  if (enable_traildrift) {
    GeoPoint tp1 = FindLatitudeLongitude(Basic().location,
                                         Calculated().wind.bearing,
                                         Calculated().wind.norm);
    traildrift = Basic().location - tp1;
  }

  fixed value_max, value_min;

  const SETTINGS_MAP &settings_map = SettingsMap();

  if (settings_map.SnailType == stAltitude) {
    value_max = fixed(1000);
    value_min = fixed(500);
    for (TracePointVector::const_iterator it = trace.begin();
         it != trace.end(); ++it) {
      value_max = max(it->GetAltitude(), value_max);
      value_min = min(it->GetAltitude(), value_min);
    }
  } else {
    value_max = fixed(0.75);
    value_min = fixed(-2.0);
    for (TracePointVector::const_iterator it = trace.begin();
         it != trace.end(); ++it) {
      value_max = max(it->GetVario(), value_max);
      value_min = min(it->GetVario(), value_min);
    }
    value_max = min(fixed(7.5), value_max);
    value_min = max(fixed(-5.0), value_min);
  }

  RasterPoint last_point;
  bool last_valid = false;
  for (TracePointVector::const_iterator it = trace.begin(), end = trace.end();
       it != end; ++it) {
    const fixed dt = Basic().time - fixed(it->time);
    const GeoPoint gp = enable_traildrift ?
      it->get_location().parametric(traildrift, dt * it->drift_factor / 256) :
      it->get_location();
    if (!bounds.inside(gp)) {
      /* the point is outside of the MapWindow; don't paint it */
      last_valid = false;
      continue;
    }

    RasterPoint pt = projection.GeoToScreen(gp);

    if (last_valid) {
      if (settings_map.SnailType == stAltitude) {
        int index = (it->GetAltitude() - value_min) / (value_max - value_min) *
                    (NUMSNAILCOLORS - 1);
        index = max(0, min(NUMSNAILCOLORS - 1, index));
        canvas.select(Graphics::hpSnail[index]);
      } else {
        const fixed colour_vario = negative(it->GetVario())
          ? - it->GetVario() / value_min
          : it->GetVario() / value_max ;

        if (!settings_map.SnailScaling ||
            projection.GetMapScale() > fixed_int_constant(6000))
          canvas.select(Graphics::hpSnail[GetSnailColorIndex(colour_vario)]);
        else
          canvas.select(Graphics::hpSnailVario[GetSnailColorIndex(colour_vario)]);
      }
      canvas.line_piece(last_point, pt);
    }
    last_point = pt;
    last_valid = true;
  }

  canvas.line(last_point, aircraft_pos);
}
