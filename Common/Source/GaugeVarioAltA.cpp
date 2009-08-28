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

#include "GaugeVarioAltA.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "MapWindowProjection.hpp"
#include "Logger.h"
#include "Math/FastMath.h"
#include "Blackboard.hpp"
#include "InfoBoxLayout.h"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Math/Geometry.hpp"
#include "McReady.h"
#include "Interface.hpp"

#include "SettingsUser.hpp"
#include "SettingsComputer.hpp"

#include <assert.h>
#include <stdlib.h>

HWND   hWndVarioWindow = NULL; // Vario Window

HDC GaugeVario::hdcScreen = NULL;
HDC GaugeVario::hdcDrawWindow = NULL;
HDC GaugeVario::hdcTemp = NULL;
HBITMAP GaugeVario::hDrawBitMap = NULL;
RECT GaugeVario::rc;
bool GaugeVario::dirty;
int GaugeVario::xoffset;
int GaugeVario::yoffset;
int GaugeVario::gmax;
POINT* GaugeVario::polys=NULL;
POINT* GaugeVario::lines=NULL;

HBITMAP GaugeVario::hBitmapUnit;
HBITMAP GaugeVario::hBitmapClimb;
POINT GaugeVario::BitmapUnitPos;
POINT GaugeVario::BitmapUnitSize;
HBRUSH GaugeVario::redBrush;
HBRUSH GaugeVario::blueBrush;
HPEN GaugeVario::redPen;
HPEN GaugeVario::bluePen;
HPEN GaugeVario::redThickPen;
HPEN GaugeVario::blueThickPen;
HPEN GaugeVario::blankThickPen;

HBRUSH GaugeVario::yellowBrush;
HBRUSH GaugeVario::greenBrush;
HBRUSH GaugeVario::magentaBrush;
HPEN GaugeVario::yellowPen;
HPEN GaugeVario::greenPen;
HPEN GaugeVario::magentaPen;


DrawInfo_t GaugeVario::diValueTop = {false};
DrawInfo_t GaugeVario::diValueMiddle = {false};
DrawInfo_t GaugeVario::diValueBottom = {false};
DrawInfo_t GaugeVario::diLabelTop = {false};
DrawInfo_t GaugeVario::diLabelMiddle = {false};
DrawInfo_t GaugeVario::diLabelBottom = {false};

#define GAUGEXSIZE (InfoBoxLayout::ControlWidth)
#define GAUGEYSIZE (InfoBoxLayout::ControlHeight*3)


static COLORREF colTextGray;
static COLORREF colText;
static COLORREF colTextBackgnd;


#define NARROWS 3
#define ARROWYSIZE IBLSCALE(3)
#define ARROWXSIZE IBLSCALE(7)


LRESULT CALLBACK GaugeVarioWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


void GaugeVario::Create() {

  StartupStore(TEXT("Create Vario\n"));

  RECT MapRectBig = MapWindowProjection::GetMapRect();

  hWndVarioWindow = CreateWindow(TEXT("STATIC"),TEXT(" "),
			       WS_VISIBLE|WS_CHILD | WS_CLIPCHILDREN
			       | WS_CLIPSIBLINGS,
                               0,0,0,0,
			       hWndMainWindow,NULL,hInst,NULL);
  if (InfoBoxLayout::landscape) {
    SetWindowPos(hWndVarioWindow, HWND_TOP,
                 MapRectBig.right+InfoBoxLayout::ControlWidth,
                 MapRectBig.top,
                 GAUGEXSIZE,GAUGEYSIZE,
                 SWP_HIDEWINDOW);
  } else {
    SetWindowPos(hWndVarioWindow, HWND_TOP,
                 MapRectBig.right-GAUGEXSIZE,
                 MapRectBig.top,
                 GAUGEXSIZE,GAUGEYSIZE,
                 SWP_HIDEWINDOW);
  }

  GetClientRect(hWndVarioWindow, &rc);

  hdcScreen = GetDC(hWndVarioWindow);       // the screen DC
  hdcDrawWindow = CreateCompatibleDC(hdcScreen);            // the memory DC
  hdcTemp = CreateCompatibleDC(hdcScreen);  // temp DC to select Uniz Bmp's

                       // prepare drawing DC, setup size and color depth
  HBITMAP memBM = CreateCompatibleBitmap (hdcScreen,
					  rc.right-rc.left,
					  rc.bottom-rc.top);
  SelectObject(hdcDrawWindow, memBM);

  // load vario scale
  if (Units::GetUserVerticalSpeedUnit()==unKnots) {
    hDrawBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_VARIOSCALEC));
  } else {
    hDrawBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_VARIOSCALEA));
  }

  COLORREF theredColor;
  COLORREF theblueColor;
  COLORREF theyellowColor; // VENTA2
  COLORREF thegreenColor; // VENTA2
  COLORREF themagentaColor; // VENTA2

  if (Appearance.InverseInfoBox) {
    theredColor = MapGfx.inv_redColor;
    theblueColor = MapGfx.inv_blueColor;
    theyellowColor = MapGfx.inv_yellowColor;
    thegreenColor = MapGfx.inv_greenColor;
    themagentaColor = MapGfx.inv_magentaColor;
  } else {
    theredColor = MapGfx.redColor;
    theblueColor = MapGfx.blueColor;
    theyellowColor = MapGfx.yellowColor;
    thegreenColor = MapGfx.greenColor;
    themagentaColor = MapGfx.magentaColor;
  }
  redBrush = CreateSolidBrush(theredColor);
  blueBrush = CreateSolidBrush(theblueColor);
  yellowBrush = CreateSolidBrush(theyellowColor);
  greenBrush = CreateSolidBrush(thegreenColor);
  magentaBrush = CreateSolidBrush(themagentaColor);
  redPen = CreatePen(PS_SOLID, 1,
                     theredColor);
  bluePen = CreatePen(PS_SOLID, 1,
                      theblueColor);
  yellowPen = CreatePen(PS_SOLID, 1,
                      theyellowColor);
  greenPen = CreatePen(PS_SOLID, 1,
                      thegreenColor);
  magentaPen = CreatePen(PS_SOLID, 1,
                      themagentaColor);
  redThickPen = CreatePen(PS_SOLID, IBLSCALE(5),
                     theredColor);
  blueThickPen = CreatePen(PS_SOLID, IBLSCALE(5),
			   theblueColor);

  if (Appearance.InverseInfoBox){
    colText = RGB(0xff, 0xff, 0xff);
    colTextBackgnd = RGB(0x00, 0x00, 0x00);
    colTextGray = RGB(0xa0, 0xa0, 0xa0);
    hBitmapClimb = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLIMBSMALLINV));
  } else {
    colText = RGB(0x00, 0x00, 0x00);
    colTextBackgnd = RGB(0xff, 0xff, 0xff);
    colTextGray = RGB(~0xa0, ~0xa0, ~0xa0);
    hBitmapClimb = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLIMBSMALL));
  }

  blankThickPen = CreatePen(PS_SOLID, IBLSCALE(5),
			    colTextBackgnd);

  SetTextColor(hdcDrawWindow, colText);
  SetBkColor(hdcDrawWindow, colTextBackgnd);

  if (Appearance.InverseInfoBox){
    Units::GetUnitBitmap(Units::GetUserUnitByGroup(ugVerticalSpeed),
      &hBitmapUnit, &BitmapUnitPos, &BitmapUnitSize, UNITBITMAPINVERS | UNITBITMAPGRAY);
  } else {
    Units::GetUnitBitmap(Units::GetUserUnitByGroup(ugVerticalSpeed),
      &hBitmapUnit, &BitmapUnitPos, &BitmapUnitSize, UNITBITMAPGRAY);
  }

  xoffset = (rc.right-rc.left);
  yoffset = (rc.bottom-rc.top)/2;

  SetWindowLong(hWndVarioWindow, GWL_WNDPROC, (LONG) GaugeVarioWndProc);

  ShowWindow(hWndVarioWindow, SW_HIDE);
}

void GaugeVario::Show(bool doshow) {
  bool gaugeVarioInPortrait = false;
#ifdef GNAV
  gaugeVarioInPortrait = true;
#endif

 // Disable vario gauge in geometry 5 landscape mode, leave 8 boxes on the right
 if ( ( InfoBoxLayout::landscape == true)
      && ( InfoBoxLayout::InfoBoxGeometry == 5 ) ) return; // VENTA3

  if (gaugeVarioInPortrait || InfoBoxLayout::landscape) {
    EnableVarioGauge = doshow;

    static bool lastvisible = false;
    if (EnableVarioGauge && !lastvisible) {
      ShowWindow(hWndVarioWindow, SW_SHOW);
    }
    if (!EnableVarioGauge && lastvisible) {
      ShowWindow(hWndVarioWindow, SW_HIDE);
    }
    lastvisible = EnableVarioGauge;
  }

}


void GaugeVario::Destroy() {
  if (polys) {
    free(polys);
    polys=NULL;
  }
  if (lines) {
    free(lines);
    lines=NULL;
  }
  ReleaseDC(hWndVarioWindow, hdcScreen);
  DeleteDC(hdcDrawWindow);
  DeleteDC(hdcTemp);
  DeleteObject(hDrawBitMap);
  DestroyWindow(hWndVarioWindow);
}


#define GAUGEVARIORANGE 5.0 //2.50 // 5 m/s
#define GAUGEVARIOSWEEP 180 // degrees total sweep

void GaugeVario::Render() {

  static POINT orgTop     = {-1,-1};
  static POINT orgMiddle  = {-1,-1};
  static POINT orgBottom  = {-1,-1};
  static int ValueHeight;
  static bool InitDone = false;

  double vval;

//  HKEY Key;

//  RenderBg();                                          // ???

  if (!InitDone){
    HBITMAP oldBmp;
    ValueHeight = (4
		   + Appearance.CDIWindowFont.CapitalHeight
		   + Appearance.TitleWindowFont.CapitalHeight);

    orgMiddle.y = yoffset - ValueHeight/2;
    orgMiddle.x = rc.right;
    orgTop.y = orgMiddle.y-ValueHeight;
    orgTop.x = rc.right;
    orgBottom.y = orgMiddle.y + ValueHeight;
    orgBottom.x = rc.right;

    oldBmp = (HBITMAP)SelectObject(hdcTemp, (HBITMAP)hDrawBitMap);
    // copy scale bitmap to memory DC
    if (InfoBoxLayout::dscale>1) {
      if (Appearance.InverseInfoBox)
	StretchBlt(hdcDrawWindow, 0, 0, rc.right, rc.bottom,
		   hdcTemp,
		   58, 0,
		   58, 120,
		   SRCCOPY);
      else
	StretchBlt(hdcDrawWindow, 0, 0, rc.right, rc.bottom,
		   hdcTemp,
		   0, 0,
		   58, 120,
		   SRCCOPY);
    } else {

      if (Appearance.InverseInfoBox)
	BitBlt(hdcDrawWindow, 0, 0, rc.right, rc.bottom,
	       hdcTemp, 58, 0, SRCCOPY);
      else
	BitBlt(hdcDrawWindow, 0, 0, rc.right, rc.bottom,
	       hdcTemp, 0, 0, SRCCOPY);

    }

    SelectObject(hdcTemp, oldBmp);

    InitDone = true;
  }

  if (GPS_INFO.VarioAvailable && !ReplayLogger::IsEnabled()) {
    vval = GPS_INFO.Vario;
  } else {
    vval = CALCULATED_INFO.Vario;
  }

  double vvaldisplay = min(99.9,max(-99.9,vval*LIFTMODIFY));

  if (Appearance.GaugeVarioAvgText) {
    // JMW averager now displays netto average if not circling
    if (!CALCULATED_INFO.Circling) {
      RenderValue(orgTop.x, orgTop.y, &diValueTop, &diLabelTop,
		  CALCULATED_INFO.NettoAverage30s*LIFTMODIFY, TEXT("NetAvg"));
    } else {
      RenderValue(orgTop.x, orgTop.y, &diValueTop, &diLabelTop,
		  CALCULATED_INFO.Average30s*LIFTMODIFY, TEXT("Avg"));
    }
  }

  if (Appearance.GaugeVarioMc) {
    double mc = GlidePolar::GetMacCready()*LIFTMODIFY;
    if (CALCULATED_INFO.AutoMacCready)
      RenderValue(orgBottom.x, orgBottom.y,
		  &diValueBottom, &diLabelBottom,
		  mc, TEXT("Auto Mc"));
    else
      RenderValue(orgBottom.x, orgBottom.y,
		  &diValueBottom, &diLabelBottom,
		  mc, TEXT("Mc"));
  }

  if (Appearance.GaugeVarioSpeedToFly) {
    RenderSpeedToFly(rc.right - 11, (rc.bottom-rc.top)/2);
  } else {
    RenderClimb();
  }

  if (Appearance.GaugeVarioBallast) {
    RenderBallast();
  }

  if (Appearance.GaugeVarioBugs) {
    RenderBugs();
  }

  dirty = false;
  int ival, sval, ival_av = 0;
  static int vval_last = 0;
  static int sval_last = 0;
  static int ival_last = 0;

  ival = ValueToNeedlePos(vval);
  sval = ValueToNeedlePos(CALCULATED_INFO.GliderSinkRate);
  if (Appearance.GaugeVarioAveNeedle) {
    if (!CALCULATED_INFO.Circling) {
      ival_av = ValueToNeedlePos(CALCULATED_INFO.NettoAverage30s);
    } else {
      ival_av = ValueToNeedlePos(CALCULATED_INFO.Average30s);
    }
  }

  // clear items first

  if (Appearance.GaugeVarioAveNeedle) {
    if (ival_av != ival_last) {
      RenderNeedle(ival_last, true, true);
    }
    ival_last = ival_av;
  }

  if ((sval != sval_last) || (ival != vval_last)) {
    RenderVarioLine(vval_last, sval_last, true);
  }
  sval_last = sval;

  if (ival != vval_last) {
    RenderNeedle(vval_last, false, true);
  }
  vval_last = ival;

  // now draw items
  RenderVarioLine(ival, sval, false);
  if (Appearance.GaugeVarioAveNeedle) {
    RenderNeedle(ival_av, true, false);
  }
  RenderNeedle(ival, false, false);

  if (Appearance.GaugeVarioGross) {
    RenderValue(orgMiddle.x, orgMiddle.y,
                &diValueMiddle, &diLabelMiddle,
                vvaldisplay,
                TEXT("Gross"));
  }
  RenderZero();

  BitBlt(hdcScreen, 0, 0, rc.right, rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);

}

void GaugeVario::Repaint(HDC hDC){
  BitBlt(hDC, 0, 0, rc.right, rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);
}

void GaugeVario::RenderBg() {
}

void GaugeVario::MakePolygon(const int i) {
  static bool InitDone = false;
  static int nlength0, nlength1, nwidth, nline;
  double dx, dy;
  POINT *bit = getPolygon(i);
  POINT *bline = &lines[i+gmax];

  if (!InitDone){
    if (Appearance.GaugeVarioNeedleStyle == gvnsLongNeedle) {
      nlength0 = IBLSCALE(15); // was 18
      nlength1 = IBLSCALE(6);
      nwidth = IBLSCALE(4);  // was 3
      nline = IBLSCALE(8);
    } else {
      nlength0 = IBLSCALE(13);
      nlength1 = IBLSCALE(6);
      nwidth = IBLSCALE(4);
      nline = IBLSCALE(8);
    }
    InitDone = true;
  }

// VENTA2 ELLIPSE for geometry 5= 1.32
#ifdef PNA

  dx = -xoffset+nlength0; dy = nwidth;
  rotate(dx, dy, i);
  bit[0].x = lround(dx+xoffset); bit[0].y = lround(dy*GlobalEllipse+yoffset+1);

  dx = -xoffset+nlength0; dy = -nwidth;
  rotate(dx, dy, i);
  bit[2].x = lround(dx+xoffset); bit[2].y = lround(dy*GlobalEllipse+yoffset+1);

  dx = -xoffset+nlength1; dy = 0;
  rotate(dx, dy, i);
  bit[1].x = lround(dx+xoffset); bit[1].y = lround(dy*GlobalEllipse+yoffset+1);

  dx = -xoffset+nline; dy = 0;
  rotate(dx, dy, i);
  bline->x = lround(dx+xoffset); bline->y = lround(dy*GlobalEllipse+yoffset+1);

#else
#define ELLIPSE 1.1

  dx = -xoffset+nlength0; dy = nwidth;
  rotate(dx, dy, i);
  bit[0].x = lround(dx+xoffset); bit[0].y = lround(dy*ELLIPSE+yoffset+1);

  dx = -xoffset+nlength0; dy = -nwidth;
  rotate(dx, dy, i);
  bit[2].x = lround(dx+xoffset); bit[2].y = lround(dy*ELLIPSE+yoffset+1);

  dx = -xoffset+nlength1; dy = 0;
  rotate(dx, dy, i);
  bit[1].x = lround(dx+xoffset); bit[1].y = lround(dy*ELLIPSE+yoffset+1);

  dx = -xoffset+nline; dy = 0;
  rotate(dx, dy, i);
  bline->x = lround(dx+xoffset); bline->y = lround(dy*ELLIPSE+yoffset+1);
#endif
}


POINT *GaugeVario::getPolygon(int i) {
  return polys+(i+gmax)*3;
}

void GaugeVario::MakeAllPolygons() {
  polys = (POINT*)malloc((gmax*2+1)*3*sizeof(POINT));
  lines = (POINT*)malloc((gmax*2+1)*sizeof(POINT));
  assert(polys);
  assert(lines);
  if (polys && lines) {
    for (int i= -gmax; i<= gmax; i++) {
      MakePolygon(i);
    }
  }
}


void GaugeVario::RenderClimb() {

  int x = rc.right-IBLSCALE(14);
  int y = rc.bottom-IBLSCALE(24);

  // testing  GPS_INFO.SwitchState.VarioCircling = true;

  if (!dirty) return;

  if (!GPS_INFO.SwitchState.VarioCircling) {
    if (Appearance.InverseInfoBox){
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    }
    SetBkMode(hdcDrawWindow, OPAQUE);

    Rectangle(hdcDrawWindow,
	      x, y,
	      x+IBLSCALE(12),y+IBLSCALE(12));
  } else {
    HBITMAP oldBmp = (HBITMAP)SelectObject(hdcTemp, hBitmapClimb);
    if (InfoBoxLayout::dscale>1) {
      StretchBlt(hdcDrawWindow,
		 x,
		 y,
		 IBLSCALE(12),
		 IBLSCALE(12),
		 hdcTemp,
		 12, 0,
		 12, 12,
		 SRCCOPY
		 );
    } else {
      BitBlt(hdcDrawWindow,
	     x,
	     y,
	     12, 12,
	     hdcTemp,
	     12, 0,
	     SRCCOPY
	     );
    }
    SelectObject(hdcTemp, oldBmp);
  }
}


void GaugeVario::RenderZero(void) {
  static POINT lp[2];
  if (Appearance.InverseInfoBox){
    SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
    SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
  } else {
    SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
    SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
  }
  lp[0].x = 0; lp[0].y = yoffset+1;
  lp[1].x = IBLSCALE(17); lp[1].y = yoffset+1;
  Polyline(hdcDrawWindow,lp,2);
  lp[0].y-= 1; lp[1].y-= 1;
  Polyline(hdcDrawWindow,lp,2);
}

int  GaugeVario::ValueToNeedlePos(double Value) {
  static bool InitDone = false;
  static int degrees_per_unit;
  int i;
  if (!InitDone){
    degrees_per_unit =
      (int)((GAUGEVARIOSWEEP/2.0)/(GAUGEVARIORANGE*LIFTMODIFY));
    gmax =
      max(80,(int)(degrees_per_unit*(GAUGEVARIORANGE*LIFTMODIFY))+2);
    MakeAllPolygons();
    InitDone = true;
  };
  i = iround(Value*degrees_per_unit*LIFTMODIFY);
  i = min(gmax,max(-gmax,i));
  return i;
}


void GaugeVario::RenderVarioLine(int i, int sink, bool clear) {
  dirty = true;
  if (i==sink) return; // nothing to do

  if (clear){
    SelectObject(hdcDrawWindow, blankThickPen);
  } else {
    if (i>sink) {
      SelectObject(hdcDrawWindow, blueThickPen);
    } else {
      SelectObject(hdcDrawWindow, redThickPen);
    }
  }
  if (i>sink) {
    Polyline(hdcDrawWindow, lines+gmax+sink, i-sink);
  } else {
    Polyline(hdcDrawWindow, lines+gmax+i, sink-i);
  }
  if (!clear) {
    // clear up naked (sink) edge of polygon, this gives it a nice
    // taper look
    if (Appearance.InverseInfoBox) {
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    }
    Polygon(hdcDrawWindow, getPolygon(sink), 3);
  }
}

void GaugeVario::RenderNeedle(int i, bool average, bool clear) {
  dirty = true;
  bool colorfull = false;

#ifdef FIVV
  colorfull = Appearance.InfoBoxColors;
#endif

  if (clear || !colorfull) {
    // legacy behaviour
    if (clear ^ Appearance.InverseInfoBox) {
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    }
  } else {
    // VENTA2-ADDON Colorful needles
    // code reorganised by JMW
    if (Appearance.InverseInfoBox) {
      if (average) {
	// Average Needle
	if ( i >= 0) {
	  SelectObject(hdcDrawWindow, greenBrush);
	  SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
	} else {
	  SelectObject(hdcDrawWindow, redBrush);
	  SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
	}
      } else {
	// varioline needle: b&w as usual, could also change aspect..
	SelectObject(hdcDrawWindow, yellowBrush );
	SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
      }
    } else {
      // Non inverse infoboxes
      if (average) {
	// Average Needle
	if ( i >= 0) {
	  SelectObject(hdcDrawWindow, greenBrush);
	  SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
	} else {
	  SelectObject(hdcDrawWindow, redBrush);
	  SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN) );
	}
      } else {
	// varioline needle: b&w as usual, could also change aspect..
	SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
	SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
      }
    }
  }

  if (average) {
    Polyline(hdcDrawWindow, getPolygon(i), 3);
  } else {
    Polygon(hdcDrawWindow, getPolygon(i), 3);
  }
}


// TODO code: Optimise vario rendering, this is slow
void GaugeVario::RenderValue(int x, int y,
			     DrawInfo_t *diValue,
			     DrawInfo_t *diLabel, double Value,
			     const TCHAR *Label) {

  SIZE tsize;

  Value = (double)iround(Value*10)/10;  // prevent the -0.0 case

  if (!diValue->InitDone){

    diValue->recBkg.right = x-IBLSCALE(5);
    diValue->recBkg.top = y +IBLSCALE(3)
      + Appearance.TitleWindowFont.CapitalHeight;

    diValue->recBkg.left = diValue->recBkg.right;
    // update back rect with max label size
    diValue->recBkg.bottom = diValue->recBkg.top
      + Appearance.CDIWindowFont.CapitalHeight;

    diValue->orgText.x = diValue->recBkg.left;
    diValue->orgText.y = diValue->recBkg.top
      + Appearance.CDIWindowFont.CapitalHeight
      - Appearance.CDIWindowFont.AscentHeight;

    diValue->lastValue = -9999;
    diValue->lastText[0] = '\0';
    diValue->lastBitMap = NULL;
    diValue->InitDone = true;
  }

  if (!diLabel->InitDone){

    diLabel->recBkg.right = x;
    diLabel->recBkg.top = y+IBLSCALE(1);

    diLabel->recBkg.left = diLabel->recBkg.right;
    // update back rect with max label size
    diLabel->recBkg.bottom = diLabel->recBkg.top
      + Appearance.TitleWindowFont.CapitalHeight;

    diLabel->orgText.x = diLabel->recBkg.left;
    diLabel->orgText.y = diLabel->recBkg.top
      + Appearance.TitleWindowFont.CapitalHeight
      - Appearance.TitleWindowFont.AscentHeight;

    diLabel->lastValue = -9999;
    diLabel->lastText[0] = '\0';
    diLabel->lastBitMap = NULL;
    diLabel->InitDone = true;
  }

  SetBkMode(hdcDrawWindow, TRANSPARENT);

  if (dirty && (_tcscmp(diLabel->lastText, Label) != 0)) {
    SetBkColor(hdcDrawWindow, colTextBackgnd);
    SetTextColor(hdcDrawWindow, colTextGray);
    SelectObject(hdcDrawWindow, TitleWindowFont);
    GetTextExtentPoint(hdcDrawWindow, Label, _tcslen(Label), &tsize);
    diLabel->orgText.x = diLabel->recBkg.right - tsize.cx;
    ExtTextOut(hdcDrawWindow, diLabel->orgText.x, diLabel->orgText.y,
	       ETO_OPAQUE, &diLabel->recBkg, Label, _tcslen(Label), NULL);
    diLabel->recBkg.left = diLabel->orgText.x;
    _tcscpy(diLabel->lastText, Label);
  }

  if (dirty && (diValue->lastValue != Value)) {
    TCHAR Temp[18];
    SetBkColor(hdcDrawWindow, colTextBackgnd);
    SetTextColor(hdcDrawWindow, colText);
    _stprintf(Temp, TEXT("%.1f"), Value);
    SelectObject(hdcDrawWindow, CDIWindowFont);
    GetTextExtentPoint(hdcDrawWindow, Temp, _tcslen(Temp), &tsize);
    diValue->orgText.x = diValue->recBkg.right - tsize.cx;

    ExtTextOut(hdcDrawWindow, diValue->orgText.x, diValue->orgText.y,
	       ETO_OPAQUE, &diValue->recBkg, Temp, _tcslen(Temp), NULL);

    diValue->recBkg.left = diValue->orgText.x;
    diValue->lastValue = Value;
  }

  if (dirty && (diLabel->lastBitMap != hBitmapUnit)) {
    HBITMAP oldBmp;

    oldBmp = (HBITMAP)SelectObject(hdcTemp, hBitmapUnit);
    if (InfoBoxLayout::dscale>1) {
      StretchBlt(hdcDrawWindow,
		 x-IBLSCALE(5),
		 diValue->recBkg.top,
		 IBLSCALE(BitmapUnitSize.x),
		 IBLSCALE(BitmapUnitSize.y),
		 hdcTemp,
		 BitmapUnitPos.x, BitmapUnitPos.y,
		 BitmapUnitSize.x, BitmapUnitSize.y,
		 SRCCOPY
		 );
    } else {
      BitBlt(hdcDrawWindow,
	     x-5,
	     diValue->recBkg.top,
	     BitmapUnitSize.x, BitmapUnitSize.y,
	     hdcTemp,
	     BitmapUnitPos.x, BitmapUnitPos.y,
	     SRCCOPY
	     );
    }
    SelectObject(hdcTemp, oldBmp);
    diLabel->lastBitMap = hBitmapUnit;
  }

}


void GaugeVario::RenderSpeedToFly(int x, int y){

  #define  YOFFSET     36
  #define  DeltaVstep  4
  #define  DeltaVlimit 16

#ifndef _SIM_
  if (!(GPS_INFO.AirspeedAvailable && GPS_INFO.VarioAvailable)) {
    return;
  }
#else
  // cheat
  GPS_INFO.IndicatedAirspeed = GPS_INFO.Speed;
#endif

  static double lastVdiff;
  double vdiff;

  int nary = NARROWS*ARROWYSIZE;
  int ytop = rc.top
    +YOFFSET+nary; // JMW
  int ybottom = rc.bottom
    -YOFFSET-nary-InfoBoxLayout::scale; // JMW

  ytop += IBLSCALE(14);
  ybottom -= IBLSCALE(14);
  // JMW
  //  x = rc.left+IBLSCALE(1);
  x = rc.right-2*ARROWXSIZE;

  // only draw speed command if flying and vario is not circling
  //
  if ((CALCULATED_INFO.Flying)
#ifdef _SIM_
      && !CALCULATED_INFO.Circling
#endif
      && !GPS_INFO.SwitchState.VarioCircling) {
    vdiff = (CALCULATED_INFO.VOpt - GPS_INFO.IndicatedAirspeed);
    vdiff = max(-DeltaVlimit, min(DeltaVlimit, vdiff)); // limit it
    vdiff = iround(vdiff/DeltaVstep) * DeltaVstep;
  } else
    vdiff = 0;

  if ((lastVdiff != vdiff)||(dirty)) {

    lastVdiff = vdiff;

    if (Appearance.InverseInfoBox){
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    }
    SetBkMode(hdcDrawWindow, OPAQUE);

    // ToDo sgi optimize

    // bottom (too slow)
    Rectangle(hdcDrawWindow,
	      x, (ybottom+YOFFSET),
	      x+ARROWXSIZE*2+1,
              (ybottom+YOFFSET)+nary+ARROWYSIZE+InfoBoxLayout::scale*2);

    // top (too fast)
    Rectangle(hdcDrawWindow,
	      x, (ytop-YOFFSET)+1,
	      x+ARROWXSIZE*2+1,
              (ytop-YOFFSET)-nary+1-ARROWYSIZE-InfoBoxLayout::scale*2);

    RenderClimb();

    if (Appearance.InverseInfoBox){
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    }

    if (Appearance.InfoBoxColors) {
      if (vdiff>0) { // too slow
        SelectObject(hdcDrawWindow, redBrush);
        SelectObject(hdcDrawWindow, redPen);
      } else {
        SelectObject(hdcDrawWindow, blueBrush);
        SelectObject(hdcDrawWindow, bluePen);
      }
    } else {
      if (Appearance.InverseInfoBox){
        SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      } else {
        SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      }
    }

    if (vdiff > 0){ // too slow

      y = ybottom;
      y += YOFFSET;

      while (vdiff > 0){
        if (vdiff > DeltaVstep){
          Rectangle(hdcDrawWindow,
		    x, y,
		    x+ARROWXSIZE*2+1, y+ARROWYSIZE-1);
        } else {
          POINT Arrow[4];
          Arrow[0].x = x; Arrow[0].y = y;
          Arrow[1].x = x+ARROWXSIZE; Arrow[1].y = y+ARROWYSIZE-1;
          Arrow[2].x = x+2*ARROWXSIZE; Arrow[2].y = y;
          Arrow[3].x = x; Arrow[3].y = y;
          Polygon(hdcDrawWindow, Arrow, 4);
        }
        vdiff -=DeltaVstep;
        y += ARROWYSIZE;
      }

    } else if (vdiff < 0){ // too fast

      y = ytop;
      y -= YOFFSET;

      while (vdiff < 0){
        if (vdiff < -DeltaVstep){
          Rectangle(hdcDrawWindow, x, y+1,
		    x+ARROWXSIZE*2+1, y-ARROWYSIZE+2);
        } else {
          POINT Arrow[4];
          Arrow[0].x = x; Arrow[0].y = y;
          Arrow[1].x = x+ARROWXSIZE; Arrow[1].y = y-ARROWYSIZE+1;
          Arrow[2].x = x+2*ARROWXSIZE; Arrow[2].y = y;
          Arrow[3].x = x; Arrow[3].y = y;
          Polygon(hdcDrawWindow, Arrow, 4);
        }
        vdiff +=DeltaVstep;
        y -= ARROWYSIZE;
      }

    }

  }

}


void GaugeVario::RenderBallast(void){

  #define TextBal TEXT("Bal")

  static double lastBallast = 1;
  static RECT  recLabelBk = {-1,-1,-1,-1};
  static RECT  recValueBk = {-1,-1,-1,-1};
  static POINT orgLabel  = {-1,-1};
  static POINT orgValue  = {-1,-1};

  if (Appearance.InverseInfoBox){
    SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
  } else {
    SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
  }

  if (recLabelBk.left == -1){                               // ontime init, origin and background rect

    SIZE tSize;

    orgLabel.x = 1;                                         // position of ballast label
    orgLabel.y = rc.top + 2 +
      (Appearance.TitleWindowFont.CapitalHeight*2)
      - Appearance.TitleWindowFont.AscentHeight;

    orgValue.x = 1;                                         // position of ballast value
    orgValue.y = rc.top + 1 +
      Appearance.TitleWindowFont.CapitalHeight
      - Appearance.TitleWindowFont.AscentHeight;

    recLabelBk.left = orgLabel.x;                           // set upper left corner
    recLabelBk.top = orgLabel.y
      + Appearance.TitleWindowFont.AscentHeight
      - Appearance.TitleWindowFont.CapitalHeight;
    recValueBk.left = orgValue.x;                           // set upper left corner
    recValueBk.top = orgValue.y
      + Appearance.TitleWindowFont.AscentHeight
      - Appearance.TitleWindowFont.CapitalHeight;

    SelectObject(hdcDrawWindow, TitleWindowFont);           // get max label size
    GetTextExtentPoint(hdcDrawWindow, TextBal, _tcslen(TextBal), &tSize);

    recLabelBk.right = recLabelBk.left + tSize.cx;          // update back rect with max label size
    recLabelBk.bottom = recLabelBk.top + Appearance.TitleWindowFont.CapitalHeight;

                                                            // get max value size
    GetTextExtentPoint(hdcDrawWindow, TEXT("100%"), _tcslen(TEXT("100%")), &tSize);

    recValueBk.right = recValueBk.left + tSize.cx;
     // update back rect with max label size
    recValueBk.bottom = recValueBk.top + Appearance.TitleWindowFont.CapitalHeight;

  }

  double BALLAST = GlidePolar::GetBallast();

  if (BALLAST != lastBallast){
       // ballast hase been changed

    TCHAR Temp[18];

    SelectObject(hdcDrawWindow, TitleWindowFont);
    SetBkColor(hdcDrawWindow, colTextBackgnd);

    if (lastBallast < 0.001 || BALLAST < 0.001){
      if (BALLAST < 0.001)    // new ballast is 0, hide label
        ExtTextOut(hdcDrawWindow,
          orgLabel.x,
          orgLabel.y,
          ETO_OPAQUE, &recLabelBk, TEXT(""), 0, NULL);
      else {
        SetTextColor(hdcDrawWindow, colTextGray);
          // ols ballast was 0, show label
        ExtTextOut(hdcDrawWindow,
          orgLabel.x,
          orgLabel.y,
          ETO_OPAQUE, &recLabelBk, TextBal, _tcslen(TextBal), NULL);
      }
    }

    if (BALLAST < 0.001)         // new ballast 0, hide value
      _stprintf(Temp, TEXT(""));
    else
      _stprintf(Temp, TEXT("%.0f%%"), BALLAST*100);

    SetTextColor(hdcDrawWindow, colText);  // display value
    ExtTextOut(hdcDrawWindow,
      orgValue.x,
      orgValue.y,
      ETO_OPAQUE, &recValueBk, Temp, _tcslen(Temp), NULL);

    lastBallast = BALLAST;

  }

}

void GaugeVario::RenderBugs(void){

  #define TextBug TEXT("Bug")
  static double lastBugs = 1;
  static RECT  recLabelBk = {-1,-1,-1,-1};
  static RECT  recValueBk = {-1,-1,-1,-1};
  static POINT orgLabel  = {-1,-1};
  static POINT orgValue  = {-1,-1};

  if (Appearance.InverseInfoBox){
    SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
  } else {
    SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
  }

  if (recLabelBk.left == -1){

    SIZE tSize;

    orgLabel.x = 1;
    orgLabel.y = rc.bottom
      - 2 - Appearance.TitleWindowFont.CapitalHeight - Appearance.TitleWindowFont.AscentHeight;

    orgValue.x = 1;
    orgValue.y = rc.bottom
      - 1 - Appearance.TitleWindowFont.AscentHeight;

    recLabelBk.left = orgLabel.x;
    recLabelBk.top = orgLabel.y
      + Appearance.TitleWindowFont.AscentHeight - Appearance.TitleWindowFont.CapitalHeight;
    recValueBk.left = orgValue.x;
    recValueBk.top = orgValue.y
      + Appearance.TitleWindowFont.AscentHeight - Appearance.TitleWindowFont.CapitalHeight;

    SelectObject(hdcDrawWindow, TitleWindowFont);
    GetTextExtentPoint(hdcDrawWindow, TextBug, _tcslen(TextBug), &tSize);

    recLabelBk.right = recLabelBk.left
      + tSize.cx;
    recLabelBk.bottom = recLabelBk.top
      + Appearance.TitleWindowFont.CapitalHeight + Appearance.TitleWindowFont.Height - Appearance.TitleWindowFont.AscentHeight;

    GetTextExtentPoint(hdcDrawWindow, TEXT("100%"), _tcslen(TEXT("100%")), &tSize);

    recValueBk.right = recValueBk.left + tSize.cx;
    recValueBk.bottom = recValueBk.top + Appearance.TitleWindowFont.CapitalHeight;

  }

  double BUGS = GlidePolar::GetBugs();

  if (BUGS != lastBugs){

    TCHAR Temp[18];

    SelectObject(hdcDrawWindow, TitleWindowFont);
    SetBkColor(hdcDrawWindow, colTextBackgnd);

    if (lastBugs > 0.999 || BUGS > 0.999){
      if (BUGS > 0.999)
        ExtTextOut(hdcDrawWindow,
          orgLabel.x,
          orgLabel.y,
          ETO_OPAQUE, &recLabelBk, TEXT(""), 0, NULL);
      else {
        SetTextColor(hdcDrawWindow, colTextGray);
        ExtTextOut(hdcDrawWindow,
          orgLabel.x,
          orgLabel.y,
          ETO_OPAQUE, &recLabelBk, TextBug, _tcslen(TextBug), NULL);
      }
    }

    if (BUGS > 0.999)
      _stprintf(Temp, TEXT(""));
    else
      _stprintf(Temp, TEXT("%.0f%%"), (1-BUGS)*100);

    SetTextColor(hdcDrawWindow, colText);

    ExtTextOut(hdcDrawWindow,
	       orgValue.x,
	       orgValue.y,
	       ETO_OPAQUE, &recValueBk, Temp, _tcslen(Temp), NULL);

    lastBugs = BUGS;

  }
}


LRESULT CALLBACK GaugeVarioWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

  PAINTSTRUCT ps;            // structure for paint info
  HDC hDC;                   // handle to graphics device context,

  switch (uMsg){
    case WM_ERASEBKGND:
      // we don't need one, we just paint over the top
    return TRUE;

    case WM_PAINT:
      if (globalRunningEvent.test() && EnableVarioGauge) {
	hDC = BeginPaint(hwnd, &ps);
	GaugeVario::Repaint(hDC);
	DeleteDC(hDC);
	EndPaint(hwnd, &ps);
      }
    break;

  }

  return(DefWindowProc (hwnd, uMsg, wParam, lParam));
}



