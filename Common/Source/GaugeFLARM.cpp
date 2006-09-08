/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/

  Copyright (C) 2000 - 2005
        M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"
#include "externs.h"
#include "GaugeFLARM.h"
#include "Utils.h"

bool EnableFLARMDisplay = true;

HWND hWndFLARMWindow = NULL; //FLARM Window

extern HWND hWndMainWindow; // Main Windows
extern HINSTANCE hInst;      // The current instance
HBITMAP GaugeFLARM::hDrawBitMap = NULL;
HBITMAP GaugeFLARM::hRoseBitMap = NULL;
HDC GaugeFLARM::hdcScreen = NULL;
HDC GaugeFLARM::hdcTemp = NULL;
HDC GaugeFLARM::hdcDrawWindow = NULL;
bool GaugeFLARM::Enable;
bool GaugeFLARM::Traffic= false;
RECT GaugeFLARM::rc;
bool GaugeFLARM::Suppress= false;

#include "InfoBoxLayout.h"
#include "XCSoar.h"
#include "Parser.h"

static COLORREF colTextGray;
static COLORREF colText;
static COLORREF colTextBackgnd;

LRESULT CALLBACK GaugeFLARMWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int GaugeFLARM::radius=0;
POINT GaugeFLARM::center;

#define FLARMMAXRANGE 2000

#include "Utils.h"

int GaugeFLARM::RangeScale(double d) {
  int rad;
  double drad = 1.0-min(1.0,d/FLARMMAXRANGE);
  rad = iround(radius*(1.0-drad*drad));
  return rad;
}

void GaugeFLARM::RenderBg() {

  SelectObject(hdcTemp, hRoseBitMap);
#if (WINDOWSPC>0)
  StretchBlt(hdcDrawWindow, 0, 0,
	     IBLSCALE(114),
	     IBLSCALE(79),
	     hdcTemp,
	     0, 0, 114, 79,
	     SRCCOPY);
#else
  BitBlt(hdcDrawWindow, 0, 0, 114, 79,
	 hdcTemp, 0, 0, SRCCOPY);
#endif

}

#include "WindowControls.h" // just to get colors

void GaugeFLARM::RenderTraffic(NMEA_INFO  *gps_info) {
  HBRUSH redBrush = CreateSolidBrush(RGB(0xFF,0x00,0x00));
  HBRUSH yellowBrush = CreateSolidBrush(RGB(0x00,0xFF,0xFF));
  HBRUSH greenBrush = CreateSolidBrush(RGB(0x00,0xFF,0x00));

  for (int i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (gps_info->FLARM_Traffic[i].ID>0) {

      switch (gps_info->FLARM_Traffic[i].AlarmLevel) {
      case 0:
	  SelectObject(hdcDrawWindow, greenBrush);
	  break;
      case 1:
	  SelectObject(hdcDrawWindow, yellowBrush);
	  break;
      case 2:
      case 3:
	  SelectObject(hdcDrawWindow, redBrush);
	  break;
      }

      // TODO: draw direction, height?
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

      rotate(x, y, -gps_info->TrackBearing); 	// or use .Heading?
      double scale = RangeScale(d);
      int targetsize = IBLSCALE(3);

      POINT shape[5];

      int xs = center.x + iround(x*scale);
      int ys = center.y + iround(y*scale);

      shape[0].y = ys-targetsize;
      shape[1].y = ys-targetsize;
      shape[2].y = ys+targetsize;
      shape[3].y = ys+targetsize;
      if (slope>=0) {
	// target aircraft is higher
	shape[0].x = xs-iround((1.0-slope)*targetsize);
	shape[1].x = xs+iround((1.0-slope)*targetsize);
	shape[2].x = xs+targetsize;
	shape[3].x = xs-targetsize;
      } else {
	// target aircraft is lower
	shape[0].x = xs-targetsize;
	shape[1].x = xs+targetsize;
	shape[2].x = xs+iround((1.0+slope)*targetsize);
	shape[3].x = xs-iround((1.0+slope)*targetsize);
      }
      shape[4].x = shape[0].x;
      shape[4].y = shape[0].y;

      Polygon(hdcDrawWindow, shape, 5);
    }
  }
  DeleteObject(greenBrush);
  DeleteObject(yellowBrush);
  DeleteObject(redBrush);
}


void GaugeFLARM::Render(NMEA_INFO *gps_info) {
  if (Enable) {
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

  Enable = false;
  Traffic = false;
  Show();
}


void GaugeFLARM::TrafficPresent(bool present) {
  Traffic = present;
}


void GaugeFLARM::Show() {
  Enable = Traffic && EnableFLARMDisplay && !Suppress;
  static bool lastvisible = true;
  if (Enable && !lastvisible) {
    ShowWindow(hWndFLARMWindow, SW_SHOW);
  }
  if (!Enable && lastvisible) {
    ShowWindow(hWndFLARMWindow, SW_HIDE);
  }
  lastvisible = Enable;
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
      if (GlobalRunning && GaugeFLARM::Enable) {
	hDC = BeginPaint(hwnd, &ps);
	GaugeFLARM::Repaint(hDC);
	DeleteDC(hDC);
	EndPaint(hwnd, &ps);
      }
    break;

  }

  return(DefWindowProc (hwnd, uMsg, wParam, lParam));
}
