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
#include "InfoBoxLayout.h"
#include "Gauge/GaugeFLARM.hpp"
#include "Math/Screen.hpp"
#include "Math/Earth.hpp"
#include "SettingsComputer.hpp"
#include "Settings.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "McReady.h"
#include "options.h" /* for IBLSCALE() */

#define fSnailColour(cv) max(0,min((short)(NUMSNAILCOLORS-1), (short)((cv+1.0)/2.0*NUMSNAILCOLORS)))

void
MapWindow::DrawFLARMTraffic(Canvas &canvas)
{

  if (!EnableFLARMMap) return;

  if (!Basic().FLARM_Available) return;

  Pen thinBlackPen(IBLSCALE(1), Color(0, 0, 0));
  POINT Arrow[5];

  canvas.select(thinBlackPen);

  int i;
//  double dX, dY;
  TextInBoxMode_t displaymode;
  displaymode.AsInt = 0;

  double screenrange = GetScreenDistanceMeters();
  double scalefact = screenrange/6000.0;

  Brush redBrush(Color(0xFF,0x00,0x00));
  Brush yellowBrush(Color(0xFF,0xFF,0x00));
  Brush greenBrush(Color(0x00,0xFF,0x00));

  const double MACCREADY = GlidePolar::GetMacCready();

  for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (Basic().FLARM_Traffic[i].ID!=0) {

      double target_lon;
      double target_lat;

      target_lon = Basic().FLARM_Traffic[i].Longitude;
      target_lat = Basic().FLARM_Traffic[i].Latitude;

      if ((EnableFLARMMap==2)&&(scalefact>1.0)) {
        double distance;
        double bearing;

        DistanceBearing(Basic().Latitude,
                        Basic().Longitude,
                        target_lat,
                        target_lon,
                        &distance,
                        &bearing);

        FindLatitudeLongitude(Basic().Latitude,
                              Basic().Longitude,
                              bearing,
                              distance*scalefact,
                              &target_lat,
                              &target_lon);

      }

      // TODO feature: draw direction, rel height?
      POINT sc, sc_name, sc_av;
      if (!LonLat2ScreenIfVisible(target_lon,
				  target_lat,
				  &sc)) {
	continue;
      }

      sc_name = sc;

      sc_name.y -= IBLSCALE(16);
      sc_av = sc_name;

#ifndef FLARM_AVERAGE
      if (Basic().FLARM_Traffic[i].Name) {
        TextInBox(hDC, Basic().FLARM_Traffic[i].Name, sc.x+IBLSCALE(3),
                  sc.y, 0, displaymode,
                  true);
      }
#else
      TCHAR label_name[100];
      TCHAR label_avg[100];

      sc_av.x += IBLSCALE(3);

      if (Basic().FLARM_Traffic[i].Name) {
	sc_name.y -= IBLSCALE(8);
	_stprintf(label_name, TEXT("%s"), Basic().FLARM_Traffic[i].Name);
      } else {
	label_name[0]= _T('\0');
      }

      if (Basic().FLARM_Traffic[i].Average30s>=0.1) {
	_stprintf(label_avg, TEXT("%.1f"),
		  LIFTMODIFY*Basic().FLARM_Traffic[i].Average30s);
      } else {
	label_avg[0]= _T('\0');
      }

#ifndef NDEBUG
      // for testing only!
      _stprintf(label_avg, TEXT("2.3"));
      _stprintf(label_name, TEXT("WUE"));
#endif

      float vmax = (float)(1.5*min(5.0, max(MACCREADY,0.5)));
      float vmin = (float)(-1.5*min(5.0, max(MACCREADY,2.0)));

      float cv = Basic().FLARM_Traffic[i].Average30s;
      if (cv<0) {
        cv /= (-vmin); // JMW fixed bug here
      } else {
        cv /= vmax;
      }

      int colourIndex = fSnailColour(cv);

      // JMW TODO enhancement: decluttering of FLARM altitudes (sort by max lift)

      int dx = (sc_av.x-Orig_Aircraft.x);
      int dy = (sc_av.y-Orig_Aircraft.y);

      if (dx*dx+dy*dy > IBLSCALE(30)*IBLSCALE(30)) {
	// only draw labels if not close to aircraft

        canvas.select(MapLabelFont);
	canvas.set_text_color(Color(0,0,0));

	if (_tcslen(label_name)>0) {
          canvas.text_opaque(sc_name.x, sc_name.y, label_name);
	}

	if (_tcslen(label_avg)>0) {
	  SIZE tsize;
	  RECT brect;

          tsize = canvas.text_size(label_avg);
	  brect.left = sc_av.x-2;
	  brect.right = brect.left+tsize.cx+6;
	  brect.top = sc_av.y+((tsize.cy+4)>>3)-2;
	  brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

          canvas.select(MapGfx.hSnailPens[colourIndex]);
          Brush hVarioBrush(MapGfx.hSnailColours[colourIndex]);
	  canvas.select(hVarioBrush);

          canvas.round_rectangle(brect.left, brect.top,
                                 brect.right, brect.bottom,
                                 IBLSCALE(8), IBLSCALE(8));

#ifdef WINDOWSPC
          canvas.background_transparent();
          canvas.text(sc_av.x, sc_av.y, label_avg);
#else
          canvas.text_opaque(sc_av.x, sc_av.y, label_avg);
#endif
	}
      }

#endif
      if ((Basic().FLARM_Traffic[i].AlarmLevel>0)
	  && (Basic().FLARM_Traffic[i].AlarmLevel<4)) {
	draw_masked_bitmap(canvas, MapGfx.hFLARMTraffic, sc.x, sc.y, 10, 10, true);
      }

      Arrow[0].x = -4;
      Arrow[0].y = 5;
      Arrow[1].x = 0;
      Arrow[1].y = -6;
      Arrow[2].x = 4;
      Arrow[2].y = 5;
      Arrow[3].x = 0;
      Arrow[3].y = 2;
      Arrow[4].x = -4;
      Arrow[4].y = 5;

      //      double vmag = max(1.0,min(15.0,Basic().FLARM_Traffic[i].Speed/5.0))*2;

      switch (Basic().FLARM_Traffic[i].AlarmLevel) {
      case 1:
	  canvas.select(yellowBrush);
	  break;
      case 2:
      case 3:
	  canvas.select(redBrush);
	  break;
      case 0:
      case 4:
	  canvas.select(greenBrush);
	  break;
      }

      PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                         Basic().FLARM_Traffic[i].TrackBearing - DisplayAngle);
      canvas.polygon(Arrow, 5);

    }
  }
}


void
MapWindow::DrawTeammate(Canvas &canvas)
{
  if (TeammateCodeValid) {
    draw_masked_bitmap_if_visible(canvas, MapGfx.hBmpTeammatePosition,
				  TeammateLongitude, TeammateLatitude,
				  20, 20);
  }
}
