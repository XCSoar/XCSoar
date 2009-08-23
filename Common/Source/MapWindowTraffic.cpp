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
#include "externs.h"
#include "GaugeFLARM.h"
#include "Math/Screen.hpp"
#include "Math/Earth.hpp"
#include "McReady.h"

extern HFONT MapLabelFont;

#define fSnailColour(cv) max(0,min((short)(NUMSNAILCOLORS-1), (short)((cv+1.0)/2.0*NUMSNAILCOLORS)))

void MapWindow::DrawFLARMTraffic(HDC hDC, RECT rc, POINT Orig_Aircraft) {

  if (!EnableFLARMMap) return;

  if (!DrawInfo.FLARM_Available) return;

  HPEN hpOld;
  HPEN thinBlackPen = CreatePen(PS_SOLID, IBLSCALE(1), RGB(0,0,0));
  POINT Arrow[5];

  hpOld = (HPEN)SelectObject(hDC, thinBlackPen);

  int i;
//  double dX, dY;
  TextInBoxMode_t displaymode;
  displaymode.AsInt = 0;

  double screenrange = GetApproxScreenRange();
  double scalefact = screenrange/6000.0;

  HBRUSH redBrush = CreateSolidBrush(RGB(0xFF,0x00,0x00));
  HBRUSH yellowBrush = CreateSolidBrush(RGB(0xFF,0xFF,0x00));
  HBRUSH greenBrush = CreateSolidBrush(RGB(0x00,0xFF,0x00));

  for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (DrawInfo.FLARM_Traffic[i].ID!=0) {

      double target_lon;
      double target_lat;

      target_lon = DrawInfo.FLARM_Traffic[i].Longitude;
      target_lat = DrawInfo.FLARM_Traffic[i].Latitude;

      if ((EnableFLARMMap==2)&&(scalefact>1.0)) {
        double distance;
        double bearing;

        DistanceBearing(DrawInfo.Latitude,
                        DrawInfo.Longitude,
                        target_lat,
                        target_lon,
                        &distance,
                        &bearing);

        FindLatitudeLongitude(DrawInfo.Latitude,
                              DrawInfo.Longitude,
                              bearing,
                              distance*scalefact,
                              &target_lat,
                              &target_lon);

      }

      // TODO feature: draw direction, rel height?
      POINT sc, sc_name, sc_av;
      LatLon2Screen(target_lon,
                    target_lat,
                    sc);

      sc_name = sc;

      sc_name.y -= IBLSCALE(16);
      sc_av = sc_name;

#ifndef FLARM_AVERAGE
      if (DrawInfo.FLARM_Traffic[i].Name) {
        TextInBox(hDC, DrawInfo.FLARM_Traffic[i].Name, sc.x+IBLSCALE(3),
                  sc.y, 0, displaymode,
                  true);
      }
#else
      TCHAR label_name[100];
      TCHAR label_avg[100];

      sc_av.x += IBLSCALE(3);

      if (DrawInfo.FLARM_Traffic[i].Name) {
	sc_name.y -= IBLSCALE(8);
	_stprintf(label_name, TEXT("%s"), DrawInfo.FLARM_Traffic[i].Name);
      } else {
	label_name[0]= _T('\0');
      }

      if (DrawInfo.FLARM_Traffic[i].Average30s>=0.1) {
	_stprintf(label_avg, TEXT("%.1f"),
		  LIFTMODIFY*DrawInfo.FLARM_Traffic[i].Average30s);
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

      float cv = DrawInfo.FLARM_Traffic[i].Average30s;
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

	HGDIOBJ oldFont = SelectObject(hDC, MapLabelFont);
	COLORREF oldTextColor = SetTextColor(hDC, RGB(0,0,0));

	if (_tcslen(label_name)>0) {
	  ExtTextOut(hDC, sc_name.x, sc_name.y, ETO_OPAQUE, NULL, label_name, _tcslen(label_name), NULL);
	}

	if (_tcslen(label_avg)>0) {
	  int size = _tcslen(label_avg);
	  SIZE tsize;
	  RECT brect;

	  GetTextExtentPoint(hDC, label_avg, size, &tsize);
	  brect.left = sc_av.x-2;
	  brect.right = brect.left+tsize.cx+6;
	  brect.top = sc_av.y+((tsize.cy+4)>>3)-2;
	  brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

	  HPEN hpOld = (HPEN)SelectObject(hDC, hSnailPens[colourIndex]);
	  HBRUSH hVarioBrush = CreateSolidBrush(hSnailColours[colourIndex]);
	  HBRUSH hbOld = (HBRUSH)SelectObject(hDC, hVarioBrush);

	  RoundRect(hDC, brect.left, brect.top, brect.right, brect.bottom,
		    IBLSCALE(8), IBLSCALE(8));

#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, sc_av.x, sc_av.y, 0, NULL, label_avg, size, NULL);
#else
      ExtTextOut(hDC, sc_av.x, sc_av.y, ETO_OPAQUE, NULL, label_avg, size, NULL);
#endif
	  SelectObject(hDC, hpOld);
	  SelectObject(hDC, hbOld);
	  DeleteObject(hVarioBrush);

	}

	SelectObject(hDC, oldFont);
	SetTextColor(hDC, oldTextColor);

      }

#endif
      if ((DrawInfo.FLARM_Traffic[i].AlarmLevel>0)
	  && (DrawInfo.FLARM_Traffic[i].AlarmLevel<4)) {
	DrawBitmapIn(hDC, sc, hFLARMTraffic);
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

      //      double vmag = max(1.0,min(15.0,DrawInfo.FLARM_Traffic[i].Speed/5.0))*2;

      switch (DrawInfo.FLARM_Traffic[i].AlarmLevel) {
      case 1:
	  SelectObject(hDC, yellowBrush);
	  break;
      case 2:
      case 3:
	  SelectObject(hDC, redBrush);
	  break;
      case 0:
      case 4:
	  SelectObject(hDC, greenBrush);
	  break;
      }

      PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                         DrawInfo.FLARM_Traffic[i].TrackBearing - DisplayAngle);
      Polygon(hDC,Arrow,5);

    }
  }

  SelectObject(hDC, hpOld);

  DeleteObject((HPEN)thinBlackPen);
  DeleteObject(greenBrush);
  DeleteObject(yellowBrush);
  DeleteObject(redBrush);

}


void MapWindow::DrawTeammate(HDC hdc, RECT rc)
{
  POINT point;

  if (TeammateCodeValid)
    {
      if(PointVisible(TeammateLongitude, TeammateLatitude) )
	{
	  LatLon2Screen(TeammateLongitude, TeammateLatitude, point);

	  SelectObject(hDCTemp,hBmpTeammatePosition);
	  DrawBitmapX(hdc,
		      point.x-IBLSCALE(10),
		      point.y-IBLSCALE(10),
		      20,20,
		      hDCTemp,0,0,SRCPAINT);

	  DrawBitmapX(hdc,
		      point.x-IBLSCALE(10),
		      point.y-IBLSCALE(10),
		      20,20,
		      hDCTemp,20,0,SRCAND);
	}
    }
}

