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

#include "GaugeFLARM.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Interface.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Screen.hpp"
#include "Compatibility/string.h"
#include "InfoBoxLayout.h"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/PaintCanvas.hpp"
#include "Screen/MainWindow.hpp"
#include <stdlib.h>

bool  EnableFLARMGauge = true;
DWORD EnableFLARMMap = 1;

PaintWindow GaugeFLARM::window; //FLARM Window

Bitmap GaugeFLARM::hRoseBitMap;
int GaugeFLARM::hRoseBitMapWidth = 0;
int GaugeFLARM::hRoseBitMapHeight= 0;
BitmapCanvas GaugeFLARM::hdcTemp;
BufferCanvas GaugeFLARM::hdcDrawWindow;
bool GaugeFLARM::Visible= false;
bool GaugeFLARM::Traffic= false;
bool GaugeFLARM::ForceVisible= false;
bool GaugeFLARM::Suppress= false;

static Color colTextGray;
static Color colText;
static Color colTextBackgnd;

LRESULT CALLBACK GaugeFLARMWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int GaugeFLARM::radius=0;
POINT GaugeFLARM::center;

#define FLARMMAXRANGE 2000

#include "Utils.h"

int GaugeFLARM::RangeScale(double d) {
  double drad = max(0.0,1.0-d/FLARMMAXRANGE);
  return iround(radius*(1.0-drad*drad));
}

void GaugeFLARM::RenderBg() {

  hdcTemp.select(hRoseBitMap);
  if ( hRoseBitMapWidth != IBLSCALE(InfoBoxLayout::ControlWidth*2)
      || hRoseBitMapHeight != IBLSCALE(InfoBoxLayout::ControlHeight*2-1) )
  {
    hdcDrawWindow.stretch(0, 0,
                          InfoBoxLayout::ControlWidth * 2,
                          InfoBoxLayout::ControlHeight * 2 - 1,
                          hdcTemp,
                          0, 0, hRoseBitMapWidth, hRoseBitMapHeight);
  }
  else
  {
    hdcDrawWindow.copy(0, 0, InfoBoxLayout::ControlWidth * 2, InfoBoxLayout::ControlHeight * 2 - 1,
                       hdcTemp, 0, 0);
  }

}

#include "WindowControls.h" // just to get colors

void GaugeFLARM::RenderTraffic(NMEA_INFO  *gps_info) {
  // TODO enhancement: support red/green Color blind pilots

  hdcDrawWindow.select(TitleWindowFont);
  hdcDrawWindow.set_text_color(Color(0x0,0x0,0x0));
  hdcDrawWindow.set_background_color(Color(0xff,0xff,0xff));

  for (int i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (gps_info->FLARM_Traffic[i].ID>0) {

      switch (gps_info->FLARM_Traffic[i].AlarmLevel) {
      case 1:
        hdcDrawWindow.select(MapGfx.yellowBrush);
	  break;
      case 2:
      case 3:
        hdcDrawWindow.select(MapGfx.redBrush);
	  break;
      case 0:
      case 4:
        hdcDrawWindow.select(MapGfx.greenBrush);
	  break;
      }

      double x, y;
      x = gps_info->FLARM_Traffic[i].RelativeEast;
      y = -gps_info->FLARM_Traffic[i].RelativeNorth;
      double d = sqrt(x*x+y*y);
      if (d>0) {
	x/= d;
	y/= d;
      } else {
	x= 0;
	y= 0;
      }
      double dh = gps_info->FLARM_Traffic[i].RelativeAltitude;
      double slope = atan2(dh,d)*2.0/3.14159; // (-1,1)

      slope = max(-1.0,min(1.0,slope*2)); // scale so 45 degrees or more=90

      double DisplayAngle = -gps_info->TrackBearing;
      rotate(x, y, DisplayAngle); 	// or use .Heading?
      double scale = RangeScale(d);

      POINT sc;
      sc.x = center.x + iround(x*scale);
      sc.y = center.y + iround(y*scale);

      if (gps_info->FLARM_Traffic[i].AlarmLevel>0) {
        // Draw line through target
        hdcDrawWindow.line(sc.x, sc.y,
                           center.x + iround(radius*x),
                           center.y + iround(radius*y));
      }

      POINT Arrow[5];

      Arrow[0].x = -3;
      Arrow[0].y = 4;
      Arrow[1].x = 0;
      Arrow[1].y = -5;
      Arrow[2].x = 3;
      Arrow[2].y = 4;
      Arrow[3].x = 0;
      Arrow[3].y = 1;
      Arrow[4].x = -3;
      Arrow[4].y = 4;

      PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                         gps_info->FLARM_Traffic[i].TrackBearing
                         + DisplayAngle);
      hdcDrawWindow.polygon(Arrow, 5);

      short relalt =
	iround(gps_info->FLARM_Traffic[i].RelativeAltitude*ALTITUDEMODIFY/100);

      if (relalt != 0) {
	TCHAR Buffer[10];
	_stprintf(Buffer, TEXT("%d"), abs(relalt));
        SIZE tsize = hdcDrawWindow.text_size(Buffer);
	tsize.cx = (tsize.cx+IBLSCALE(6))/2;
        hdcDrawWindow.text(sc.x - tsize.cx + IBLSCALE(7),
                           sc.y - tsize.cy - IBLSCALE(5),
                           Buffer);
        hdcDrawWindow.black_brush();
	POINT triangle[4];
	triangle[0].x = 3;  // was  2
	triangle[0].y = -3; // was -2
	triangle[1].x = 6;  // was 4
	triangle[1].y = 1;
	triangle[2].x = 0;
	triangle[2].y = 1;
	short flip = 1;
	if (relalt<0) {
	  flip = -1;
	}
	for (int j=0; j<3; j++) {
	  triangle[j].x = sc.x+IBLSCALE(triangle[j].x)-tsize.cx;
	  triangle[j].y = sc.y+flip*IBLSCALE(triangle[j].y)
	    -tsize.cy/2-IBLSCALE(5);
          }
	triangle[3].x = triangle[0].x;
	triangle[3].y = triangle[0].y;
        hdcDrawWindow.polygon(triangle, 4);

      }
    }
  }
}


void GaugeFLARM::Render(NMEA_INFO *gps_info) {
  if (Visible) {
    RenderBg();

    RenderTraffic(gps_info);

    window.get_canvas().copy(hdcDrawWindow);
  }
}


void GaugeFLARM::Create() {
  // start of new code for displaying FLARM window

  RECT rc = hWndMainWindow.get_client_rect();

  window.set(hWndMainWindow,
             (int)(rc.right - InfoBoxLayout::ControlWidth * 2)+1,
             (int)(rc.bottom - InfoBoxLayout::ControlHeight * 2)+1,
             (int)(InfoBoxLayout::ControlWidth * 2)-1,
             (int)(InfoBoxLayout::ControlHeight * 2)-1,
             false, false, false);
  window.insert_after(HWND_TOP, false);

  rc = window.get_client_rect();

  center.x = window.get_hmiddle();
  center.y = window.get_vmiddle();
  radius = min(window.get_right() - center.x, window.get_bottom() - center.y);

  hdcDrawWindow.set(window.get_canvas());
  hdcTemp.set(hdcDrawWindow);

  hRoseBitMap.load(IDB_FLARMROSE);

  const SIZE size = hRoseBitMap.get_size();
  hRoseBitMapWidth = size.cx;
  hRoseBitMapHeight = size.cy;

  if (Appearance.InverseInfoBox){
    colText = Color(0xff, 0xff, 0xff);
    colTextBackgnd = Color(0x00, 0x00, 0x00);
    colTextGray = Color(0xa0, 0xa0, 0xa0);
  } else {
    colText = Color(0x00, 0x00, 0x00);
    colTextBackgnd = Color(0xff, 0xff, 0xff);
    colTextGray = Color(~0xa0, ~0xa0, ~0xa0);
  }

  hdcDrawWindow.set_text_color(colText);
  hdcDrawWindow.set_background_color(colTextBackgnd);

  // end of new code for drawing FLARM window (see below for destruction of objects)

  // turn off suppression
  Suppress = false;

  window.set_wndproc(GaugeFLARMWndProc);
  window.hide();

  RenderBg();
  window.get_canvas().copy(hdcDrawWindow);

  Visible = false;
  Traffic = false;
  Show();
}


void GaugeFLARM::TrafficPresent(bool present) {
  Traffic = present;
}


void GaugeFLARM::Show() {
  Visible = ForceVisible || (Traffic && EnableFLARMGauge && !Suppress);
  static bool lastvisible = true;
  if (Visible && !lastvisible) {
    window.show();
  }
  if (!Visible && lastvisible) {
    window.hide();
  }
  lastvisible = Visible;
}


void GaugeFLARM::Destroy() {
  hdcDrawWindow.reset();
  hdcTemp.reset();
  window.reset();
}

void GaugeFLARM::Repaint(Canvas &canvas) {
  canvas.copy(hdcDrawWindow);
}

LRESULT CALLBACK GaugeFLARMWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
  switch (uMsg){

    case WM_ERASEBKGND:
      // we don't need one, we just paint over the top
    return TRUE;

    case WM_PAINT:
      if (globalRunningEvent.test() && GaugeFLARM::Visible) {
        PaintCanvas canvas(GaugeFLARM::window, hwnd);
        GaugeFLARM::Repaint(canvas);
      }
    break;

  }

  return(DefWindowProc (hwnd, uMsg, wParam, lParam));
}
