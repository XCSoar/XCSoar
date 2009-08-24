/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "MapWindow.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Utils.h"
#include "Math/Earth.hpp"
#include "externs.h"


void MapWindow::ScanVisibility(rectObj *bounds_active) {
  // received when the SetTopoBounds determines the visibility
  // boundary has changed.
  // This happens rarely, so it is good pre-filtering of what is visible.
  // (saves from having to do it every screen redraw)

  ScanVisibilityTrail(bounds_active);
  ScanVisibilityWaypoints(bounds_active);
  ScanVisibilityAirspace(bounds_active);
}


#include "SnailTrail.hpp"
// only needed once here!
// TODO: chop this into sections

void MapWindow::CalculateScreenPositions(POINT Orig, RECT rc,
                                         POINT *Orig_Aircraft)
{
  unsigned int i;

  Orig_Screen = Orig;

  if (!EnablePan) {

    if (GliderCenter
        && DerivedDrawInfo.Circling
        && (EnableThermalLocator==2)) {

      if (DerivedDrawInfo.ThermalEstimate_R>0) {
        PanLongitude = DerivedDrawInfo.ThermalEstimate_Longitude;
        PanLatitude = DerivedDrawInfo.ThermalEstimate_Latitude;
        // TODO enhancement: only pan if distance of center to
        // aircraft is smaller than one third screen width

        POINT screen;
        LatLon2Screen(PanLongitude,
                      PanLatitude,
                      screen);

        LatLon2Screen(DrawInfo.Longitude,
                      DrawInfo.Latitude,
                      *Orig_Aircraft);

        if ((fabs((double)Orig_Aircraft->x-screen.x)<(rc.right-rc.left)/3)
            && (fabs((double)Orig_Aircraft->y-screen.y)<(rc.bottom-rc.top)/3)) {

        } else {
          // out of bounds, center on aircraft
          PanLongitude = DrawInfo.Longitude;
          PanLatitude = DrawInfo.Latitude;
        }
      } else {
        PanLongitude = DrawInfo.Longitude;
        PanLatitude = DrawInfo.Latitude;
      }
    } else {
      // Pan is off
      PanLongitude = DrawInfo.Longitude;
      PanLatitude = DrawInfo.Latitude;
    }
  }

  LatLon2Screen(DrawInfo.Longitude,
                DrawInfo.Latitude,
                *Orig_Aircraft);

  screenbounds_latlon = CalculateScreenBounds(0.0);

  // get screen coordinates for all task waypoints
  CalculateScreenPositionsWaypoints();
  CalculateScreenPositionsTask();

  LockTaskData();
  if(TrailActive)
  {
    iSnailNext = SnailNext;
    // set this so that new data doesn't arrive between calculating
    // this and the screen updates
  }
  UnlockTaskData();

}

