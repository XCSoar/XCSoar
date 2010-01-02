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

#include "MapWindow.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Util.hpp"
#include "MacCready.h"
#include "Screen/Graphics.hpp"

#include "RasterTerrain.h" // OLD_TASK just for locking

#include <math.h>

#ifndef _MSC_VER
#include <algorithm>
using std::min;
using std::max;
#endif

#define fSnailColour(cv) max(0,min(NUMSNAILCOLORS-1, ((cv+fixed_one)*fixed_half*NUMSNAILCOLORS).as_int()))


void
MapWindow::DrawTrail(Canvas &canvas)
{
  if(!SettingsMap().TrailActive)
    return;

  unsigned min_time = 0;

  switch(SettingsMap().TrailActive) {
  case 1:
    min_time = max(0,(Basic().Time-3600).as_int());
    break;
  case 2:
    min_time = max(0,(Basic().Time-600).as_int());
    break;
  case 0:
    min_time = 0; // full
    break;
  }; 

  PeriodClock clock;
  clock.update();

  terrain->Lock(); 
  TracePointVector trace = task->find_trace_points(GetPanLocation(),
                                                   fixed(GetScreenDistanceMeters()), 
                                                   min_time, 
                                                   fixed(DistancePixelsToMeters(3)));
  terrain->Unlock();

  printf("A: %d\n", clock.elapsed());
  clock.update();

  if (trace.empty()) return; // nothing to draw

  GEOPOINT traildrift;

  if (SettingsMap().EnableTrailDrift && (DisplayMode == dmCircling)) {
    GEOPOINT tp1;

    FindLatitudeLongitude(Basic().Location,
                          Basic().WindDirection,
                          Basic().WindSpeed,
                          &tp1);
    traildrift = Basic().Location-tp1;
  }

  fixed vario_max = fixed(0.75);
  fixed vario_min = fixed(-2.0);

  for (TracePointVector::const_iterator it = trace.begin();
       it != trace.end(); ++it) {
    vario_max = max(it->NettoVario, vario_max);
    vario_min = min(it->NettoVario, vario_min);
  };

  vario_max = min(fixed(7.5), vario_max);
  vario_min = max(fixed(-5.0), vario_min);

  unsigned last_time = 0;

  for (TracePointVector::const_iterator it = trace.begin();
       it != trace.end(); ++it) {

    POINT pt;
    const fixed dt = (Basic().Time-it->time)*it->drift_factor;
    LonLat2Screen(it->get_location().parametric(traildrift, dt), pt);

    if (it->last_time != last_time) {
      canvas.move_to(pt.x, pt.y);
    } else {

      const fixed colour_vario = negative(it->NettoVario)?
        -it->NettoVario/vario_min :
        it->NettoVario/vario_max ;

      canvas.select(MapGfx.hSnailPens[fSnailColour(colour_vario)]);
      canvas.line_to(pt.x, pt.y);
    }
    last_time = it->time;
  }
  canvas.line_to(Orig_Aircraft.x, Orig_Aircraft.y);

  printf("B: %d\n", clock.elapsed());

}


