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
#include "externs.h"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Screen.hpp"

#include <stdlib.h>

extern HFONT  TitleWindowFont;

bool EnableFLARMGauge = true;
DWORD EnableFLARMMap = 1;

HWND GaugeFLARM::hWndFLARMWindow = NULL; //FLARM Window

HBITMAP GaugeFLARM::hDrawBitMap = NULL;
HBITMAP GaugeFLARM::hRoseBitMap = NULL;
int GaugeFLARM::hRoseBitMapWidth = 0;
int GaugeFLARM::hRoseBitMapHeight= 0;
HDC GaugeFLARM::hdcScreen = NULL;
HDC GaugeFLARM::hdcTemp = NULL;
HDC GaugeFLARM::hdcDrawWindow = NULL;
bool GaugeFLARM::Visible= false;
bool GaugeFLARM::Traffic= false;
bool GaugeFLARM::ForceVisible= false;
RECT GaugeFLARM::rc;
bool GaugeFLARM::Suppress= false;

#include "InfoBoxLayout.h"
#include "XCSoar.h"

static COLORREF colTextGray;
static COLORREF colText;
static COLORREF colTextBackgnd;

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

  SelectObject(hdcTemp, hRoseBitMap);
  if ( hRoseBitMapWidth != IBLSCALE(InfoBoxLayout::ControlWidth*2)
      || hRoseBitMapHeight != IBLSCALE(InfoBoxLayout::ControlHeight*2-1) )
  {
    StretchBlt(hdcDrawWindow, 0, 0,
	     (InfoBoxLayout::ControlWidth*2),
	     (InfoBoxLayout::ControlHeight*2-1),
	     hdcTemp,
	     0, 0, hRoseBitMapWidth, hRoseBitMapHeight,
	     SRCCOPY);
  }
  else
  {
    BitBlt(hdcDrawWindow, 0, 0, InfoBoxLayout::ControlWidth*2, InfoBoxLayout::ControlHeight*2-1,
	   hdcTemp, 0, 0, SRCCOPY);
  }

}

#include "WindowControls.h" // just to get colors

void GaugeFLARM::RenderTraffic(NMEA_INFO  *gps_info) {
  HBRUSH redBrush = CreateSolidBrush(RGB(0xFF,0x00,0x00));
  HBRUSH yellowBrush = CreateSolidBrush(RGB(0xFF,0xFF,0x00));
  HBRUSH greenBrush = CreateSolidBrush(RGB(0x00,0xFF,0x00));
  // TODO enhancement: support red/green Color blind pilots

  SelectObject(hdcDrawWindow, TitleWindowFont);
  SetTextColor(hdcDrawWindow, RGB(0x0,0x0,0x0));
  SetBkColor(hdcDrawWindow, RGB(0xff,0xff,0xff));

  for (int i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (gps_info->FLARM_Traffic[i].ID>0) {

      switch (gps_info->FLARM_Traffic[i].AlarmLevel) {
      case 1:
	  SelectObject(hdcDrawWindow, yellowBrush);
	  break;
      case 2:
      case 3:
	  SelectObject(hdcDrawWindow, redBrush);
	  break;
      case 0:
      case 4:
	  SelectObject(hdcDrawWindow, greenBrush);
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
        POINT tl[2];
        tl[0].x = sc.x;
        tl[0].y = sc.y;
        tl[1].x = center.x + iround(radius*x);
        tl[1].y = center.y + iround(radius*y);
        Polygon(hdcDrawWindow, tl, 2);
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

      short relalt =
	iround(gps_info->FLARM_Traffic[i].RelativeAltitude*ALTITUDEMODIFY/100);

      if (relalt != 0) {
	TCHAR Buffer[10];
	_stprintf(Buffer, TEXT("%d"), abs(relalt));
	short size = _tcslen(Buffer);
	SIZE tsize;
	GetTextExtentPoint(hdcDrawWindow, Buffer, size, &tsize);
	tsize.cx = (tsize.cx+IBLSCALE(6))/2;
	ExtTextOut(hdcDrawWindow, sc.x-tsize.cx+IBLSCALE(7),
		   sc.y-tsize.cy-IBLSCALE(5),
		   ETO_OPAQUE, NULL, Buffer, size, NULL);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdcDrawWindow,
					       GetStockObject(BLACK_BRUSH));
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
	Polygon(hdcDrawWindow, triangle, 4);
	SelectObject(hdcDrawWindow, oldBrush);

      }

      PolygonRotateShift(Arrow, 5, sc.x, sc.y,
			 gps_info->FLARM_Traffic[i].TrackBearing
			 + DisplayAngle);
      Polygon(hdcDrawWindow, Arrow, 5);

    }
  }
  DeleteObject(greenBrush);
  DeleteObject(yellowBrush);
  DeleteObject(redBrush);
}


void GaugeFLARM::Render(NMEA_INFO *gps_info) {
  if (Visible) {
    RenderBg();

    RenderTraffic(gps_info);

    BitBlt(hdcScreen, 0, 0, rc.right, rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);
  }
}


void GaugeFLARM::Create() {
  // start of new code for displaying FLARM window

  GetClientRect(hWndMainWindow, &rc);

  hWndFLARMWindow = CreateWindow(TEXT("STATIC"),TEXT(" "),
			       WS_VISIBLE|WS_CHILD | WS_CLIPCHILDREN
			       | WS_CLIPSIBLINGS,
			       0,0,0,0,
			       hWndMainWindow,NULL,hInst,NULL);

  SetWindowPos(hWndFLARMWindow, HWND_TOP,
	       (int)(rc.right-InfoBoxLayout::ControlWidth*2)+1,
	       (int)(rc.bottom-InfoBoxLayout::ControlHeight*2)+1,
	       (int)(InfoBoxLayout::ControlWidth*2)-1,
	       (int)(InfoBoxLayout::ControlHeight*2)-1,
	       SWP_HIDEWINDOW);

  GetClientRect(hWndFLARMWindow, &rc);

  center.x = (rc.right+rc.left)/2;
  center.y = (rc.top+rc.bottom)/2;
  radius = min(rc.right-center.x,rc.bottom-center.y);

  hdcScreen = GetDC(hWndFLARMWindow);                       // the screen DC
  hdcDrawWindow = CreateCompatibleDC(hdcScreen);            // the memory DC
  hdcTemp = CreateCompatibleDC(hdcDrawWindow);

  hRoseBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FLARMROSE));

  BITMAP bm;
  ::GetObject( hRoseBitMap, sizeof( bm ), &bm );
  hRoseBitMapWidth = bm.bmWidth;
  hRoseBitMapHeight = bm.bmHeight;


  hDrawBitMap =
    CreateCompatibleBitmap (hdcScreen, rc.right-rc.left, rc.bottom-rc.top);
  SelectObject(hdcDrawWindow, hDrawBitMap);

  if (Appearance.InverseInfoBox){
    colText = RGB(0xff, 0xff, 0xff);
    colTextBackgnd = RGB(0x00, 0x00, 0x00);
    colTextGray = RGB(0xa0, 0xa0, 0xa0);
  } else {
    colText = RGB(0x00, 0x00, 0x00);
    colTextBackgnd = RGB(0xff, 0xff, 0xff);
    colTextGray = RGB(~0xa0, ~0xa0, ~0xa0);
  }

  SetTextColor(hdcDrawWindow, colText);
  SetBkColor(hdcDrawWindow, colTextBackgnd);

  // end of new code for drawing FLARM window (see below for destruction of objects)

  // turn off suppression
  Suppress = false;

  SetWindowLong(hWndFLARMWindow, GWL_WNDPROC, (LONG) GaugeFLARMWndProc);
  ShowWindow(hWndFLARMWindow, SW_HIDE);

  RenderBg();
  BitBlt(hdcScreen, 0, 0, rc.right, rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);

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
    ShowWindow(hWndFLARMWindow, SW_SHOW);
  }
  if (!Visible && lastvisible) {
    ShowWindow(hWndFLARMWindow, SW_HIDE);
  }
  lastvisible = Visible;
}


void GaugeFLARM::Destroy() {
  ReleaseDC(hWndFLARMWindow, hdcScreen);
  DeleteDC(hdcDrawWindow);
  DeleteDC(hdcTemp);
  DeleteObject(hDrawBitMap);
  DeleteObject(hRoseBitMap);
  DestroyWindow(hWndFLARMWindow);
}

void GaugeFLARM::Repaint(HDC hDC) {
  BitBlt(hDC, 0, 0, rc.right, rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);
}

LRESULT CALLBACK GaugeFLARMWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

  PAINTSTRUCT ps;            // structure for paint info
  HDC hDC;                   // handle to graphics device context,

  switch (uMsg){

    case WM_ERASEBKGND:
      // we don't need one, we just paint over the top
    return TRUE;

    case WM_PAINT:
      if (GlobalRunning && GaugeFLARM::Visible) {
	hDC = BeginPaint(hwnd, &ps);
	GaugeFLARM::Repaint(hDC);
	DeleteDC(hDC);
	EndPaint(hwnd, &ps);
      }
    break;

  }

  return(DefWindowProc (hwnd, uMsg, wParam, lParam));
}
