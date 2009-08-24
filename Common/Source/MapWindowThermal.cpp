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
#include "Settings.hpp"
#include "SettingsUser.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "XCSoar.h"
#include "InfoBoxLayout.h"
#include "Math/FastMath.h"
#include "Math/Screen.hpp"
#include "Math/Earth.hpp"
#include "McReady.h"


void MapWindow::CalculateScreenPositionsThermalSources() {
  for (int i=0; i<MAX_THERMAL_SOURCES; i++) {
    if (DerivedDrawInfo.ThermalSources[i].LiftRate>0) {
      double dh = DerivedDrawInfo.NavAltitude
        -DerivedDrawInfo.ThermalSources[i].GroundHeight;
      if (dh<0) {
        DerivedDrawInfo.ThermalSources[i].Visible = false;
        continue;
      }

      double t = dh/DerivedDrawInfo.ThermalSources[i].LiftRate;
      double lat, lon;
      FindLatitudeLongitude(DerivedDrawInfo.ThermalSources[i].Latitude,
                            DerivedDrawInfo.ThermalSources[i].Longitude,
                            DerivedDrawInfo.WindBearing,
                            -DerivedDrawInfo.WindSpeed*t,
                            &lat, &lon);
      if (PointVisible(lon,lat)) {
        LatLon2Screen(lon,
                      lat,
                      DerivedDrawInfo.ThermalSources[i].Screen);
        DerivedDrawInfo.ThermalSources[i].Visible =
          PointVisible(DerivedDrawInfo.ThermalSources[i].Screen);
      } else {
        DerivedDrawInfo.ThermalSources[i].Visible = false;
      }
    } else {
      DerivedDrawInfo.ThermalSources[i].Visible = false;
    }
  }
}




void MapWindow::DrawThermalEstimate(HDC hdc, const RECT rc) {
  POINT screen;
  if (!EnableThermalLocator)
    return;

  if (DisplayMode == dmCircling) {
    if (DerivedDrawInfo.ThermalEstimate_R>0) {
      LatLon2Screen(DerivedDrawInfo.ThermalEstimate_Longitude,
                    DerivedDrawInfo.ThermalEstimate_Latitude,
                    screen);
      DrawBitmapIn(hdc,
		   screen,
		   hBmpThermalSource);
      /*
	SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	Circle(hdc,
	screen.x,
	screen.y, IBLSCALE(5), rc);
      */
    }
  } else {
    if (MapScale <= 4) {
      for (int i=0; i<MAX_THERMAL_SOURCES; i++) {
        if (DerivedDrawInfo.ThermalSources[i].Visible) {
          DrawBitmapIn(hdc,
                       DerivedDrawInfo.ThermalSources[i].Screen,
                       hBmpThermalSource);
        }
      }
    }
  }
}


void MapWindow::DrawThermalBand(HDC hDC, const RECT rc)
{
  POINT GliderBand[5] = { {0,0},{23,0},{22,0},{24,0},{0,0} };

  if ((DerivedDrawInfo.TaskAltitudeDifference>50)
      &&(DisplayMode == dmFinalGlide)) {
    return;
  }

  // JMW TODO accuracy: gather proper statistics
  // note these should/may also be relative to ground
  int i;
  double mth = DerivedDrawInfo.MaxThermalHeight;
  double maxh, minh;
  double h;
  double Wt[NUMTHERMALBUCKETS];
  double ht[NUMTHERMALBUCKETS];
  double Wmax=0.0;
  int TBSCALEY = ( (rc.bottom - rc.top )/2)-IBLSCALE(30);
#define TBSCALEX 20

  // calculate height above safety altitude
  double hoffset = SAFETYALTITUDEBREAKOFF+DerivedDrawInfo.TerrainBase;
  h = DerivedDrawInfo.NavAltitude-hoffset;

  bool draw_start_height = ((ActiveWayPoint==0) && (ValidTaskPoint(0))
			    && (StartMaxHeight!=0)
			    && (DerivedDrawInfo.TerrainValid));
  double hstart=0;
  if (draw_start_height) {
    if (StartHeightRef == 0) {
      hstart = StartMaxHeight+DerivedDrawInfo.TerrainAlt;
    } else {
      hstart = StartMaxHeight;
    }
    hstart -= hoffset;
  }

  // calculate top/bottom height
  maxh = max(h, mth);
  minh = min(h, 0);

  if (draw_start_height) {
    maxh = max(maxh, hstart);
    minh = min(minh, hstart);
  }

  // no thermalling has been done above safety altitude
  if (mth<=1) {
    return;
  }
  if (maxh-minh<=0) {
    return;
  }

  // normalised heights
  double hglider = (h-minh)/(maxh-minh);
  hstart = (hstart-minh)/(maxh-minh);

  // calculate averages
  int numtherm = 0;

  double mc = MACCREADY;
  Wmax = max(0.5,mc);

  for (i=0; i<NUMTHERMALBUCKETS; i++) {
    double wthis = 0;
    // height of this thermal point [0,mth]
    double hi = i*mth/NUMTHERMALBUCKETS;
    double hp = ((hi-minh)/(maxh-minh));

    if (DerivedDrawInfo.ThermalProfileN[i]>5) {
      // now requires 10 items in bucket before displaying,
      // to eliminate kinks
      wthis = DerivedDrawInfo.ThermalProfileW[i]
                 /DerivedDrawInfo.ThermalProfileN[i];
    }
    if (wthis>0.0) {
      ht[numtherm]= hp;
      Wt[numtherm]= wthis;
      Wmax = max(Wmax,wthis/1.5);
      numtherm++;
    }
  }

  if ((!draw_start_height) && (numtherm<=1)) {
    return; // don't display if insufficient statistics
    // but do draw if start height needs to be drawn
  }

  // drawing info
  HPEN hpOld;

  // position of thermal band
  if (numtherm>1) {
    hpOld = (HPEN)SelectObject(hDC, hpThermalBand);
    HBRUSH hbOld = (HBRUSH)SelectObject(hDC, hbThermalBand);

    POINT ThermalProfile[NUMTHERMALBUCKETS+2];
    for (i=0; i<numtherm; i++) {
      ThermalProfile[1+i].x =
	(iround((Wt[i]/Wmax)*IBLSCALE(TBSCALEX)))+rc.left;

      ThermalProfile[1+i].y =
	IBLSCALE(4)+iround(TBSCALEY*(1.0-ht[i]))+rc.top;
    }
    ThermalProfile[0].x = rc.left;
    ThermalProfile[0].y = ThermalProfile[1].y;
    ThermalProfile[numtherm+1].x = rc.left;
    ThermalProfile[numtherm+1].y = ThermalProfile[numtherm].y;

    Polygon(hDC,ThermalProfile,numtherm+2);
    SelectObject(hDC, hbOld);
  }

  // position of thermal band

  GliderBand[0].y = IBLSCALE(4)+iround(TBSCALEY*(1.0-hglider))+rc.top;
  GliderBand[1].y = GliderBand[0].y;
  GliderBand[1].x = max(iround((mc/Wmax)*IBLSCALE(TBSCALEX)),IBLSCALE(4))
    +rc.left;

  GliderBand[2].x = GliderBand[1].x-IBLSCALE(4);
  GliderBand[2].y = GliderBand[0].y-IBLSCALE(4);
  GliderBand[3].x = GliderBand[1].x;
  GliderBand[3].y = GliderBand[1].y;
  GliderBand[4].x = GliderBand[1].x-IBLSCALE(4);
  GliderBand[4].y = GliderBand[0].y+IBLSCALE(4);

  hpOld = (HPEN)SelectObject(hDC, hpThermalBandGlider);

  Polyline(hDC,GliderBand, 2);
  Polyline(hDC,GliderBand+2, 3); // arrow head

  if (draw_start_height) {
    SelectObject(hDC, hpFinalGlideBelow);
    GliderBand[0].y = IBLSCALE(4)+iround(TBSCALEY*(1.0-hstart))+rc.top;
    GliderBand[1].y = GliderBand[0].y;
    Polyline(hDC, GliderBand, 2);
  }

  SelectObject(hDC, hpOld);

}
