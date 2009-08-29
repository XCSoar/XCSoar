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

#include "Gauge/GaugeVarioAltA.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "MapWindowProjection.hpp"
#include "ReplayLogger.hpp"
#include "Math/FastMath.h"
#include "Blackboard.hpp"
#include "InfoBoxLayout.h"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "Screen/PaintCanvas.hpp"
#include "Screen/MainWindow.hpp"
#include "Math/Geometry.hpp"
#include "McReady.h"
#include "Interface.hpp"

#include "SettingsUser.hpp"
#include "SettingsComputer.hpp"

#include <assert.h>
#include <stdlib.h>

PaintWindow GaugeVario::window;
BufferCanvas GaugeVario::hdcDrawWindow;

Bitmap GaugeVario::hDrawBitMap;
bool GaugeVario::dirty;
int GaugeVario::xoffset;
int GaugeVario::yoffset;
int GaugeVario::gmax;
POINT* GaugeVario::polys=NULL;
POINT* GaugeVario::lines=NULL;

const Bitmap *GaugeVario::hBitmapUnit;
Bitmap GaugeVario::hBitmapClimb;
POINT GaugeVario::BitmapUnitPos;
POINT GaugeVario::BitmapUnitSize;
Brush GaugeVario::redBrush;
Brush GaugeVario::blueBrush;
Pen GaugeVario::redPen;
Pen GaugeVario::bluePen;
Pen GaugeVario::redThickPen;
Pen GaugeVario::blueThickPen;
Pen GaugeVario::blankThickPen;

Brush GaugeVario::yellowBrush;
Brush GaugeVario::greenBrush;
Brush GaugeVario::magentaBrush;
Pen GaugeVario::yellowPen;
Pen GaugeVario::greenPen;
Pen GaugeVario::magentaPen;


DrawInfo_t GaugeVario::diValueTop = {false};
DrawInfo_t GaugeVario::diValueMiddle = {false};
DrawInfo_t GaugeVario::diValueBottom = {false};
DrawInfo_t GaugeVario::diLabelTop = {false};
DrawInfo_t GaugeVario::diLabelMiddle = {false};
DrawInfo_t GaugeVario::diLabelBottom = {false};

#define GAUGEXSIZE (InfoBoxLayout::ControlWidth)
#define GAUGEYSIZE (InfoBoxLayout::ControlHeight*3)


static Color colTextGray;
static Color colText;
static Color colTextBackgnd;


#define NARROWS 3
#define ARROWYSIZE IBLSCALE(3)
#define ARROWXSIZE IBLSCALE(7)


LRESULT CALLBACK GaugeVarioWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


void GaugeVario::Create() {

  StartupStore(TEXT("Create Vario\n"));

  RECT MapRectBig = MapWindowProjection::GetMapRect();

  window.set(hWndMainWindow,
             InfoBoxLayout::landscape
             ? (MapRectBig.right + InfoBoxLayout::ControlWidth)
             : (MapRectBig.right - GAUGEXSIZE),
             MapRectBig.top,
             GAUGEXSIZE, GAUGEYSIZE,
             false, false, false);
  window.insert_after(HWND_TOP, false);

  hdcDrawWindow.set(window.get_canvas());

  // load vario scale
  if (Units::GetUserVerticalSpeedUnit()==unKnots) {
    hDrawBitMap.load(IDB_VARIOSCALEC);
  } else {
    hDrawBitMap.load(IDB_VARIOSCALEA);
  }

  Color theredColor;
  Color theblueColor;
  Color theyellowColor; // VENTA2
  Color thegreenColor; // VENTA2
  Color themagentaColor; // VENTA2

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

  redBrush.set(theredColor);
  blueBrush.set(theblueColor);
  yellowBrush.set(theyellowColor);
  greenBrush.set(thegreenColor);
  magentaBrush.set(themagentaColor);
  redPen.set(1, theredColor);
  bluePen.set(1, theblueColor);
  yellowPen.set(1, theyellowColor);
  greenPen.set(1, thegreenColor);
  magentaPen.set(1, themagentaColor);
  redThickPen.set(IBLSCALE(5), theredColor);
  blueThickPen.set(IBLSCALE(5), theblueColor);

  if (Appearance.InverseInfoBox){
    colText = Color(0xff, 0xff, 0xff);
    colTextBackgnd = Color(0x00, 0x00, 0x00);
    colTextGray = Color(0xa0, 0xa0, 0xa0);
    hBitmapClimb.load(IDB_CLIMBSMALLINV);
  } else {
    colText = Color(0x00, 0x00, 0x00);
    colTextBackgnd = Color(0xff, 0xff, 0xff);
    colTextGray = Color(~0xa0, ~0xa0, ~0xa0);
    hBitmapClimb.load(IDB_CLIMBSMALL);
  }

  blankThickPen.set(IBLSCALE(5), colTextBackgnd);

  hdcDrawWindow.set_text_color(colText);
  hdcDrawWindow.set_background_color(colTextBackgnd);

  if (Appearance.InverseInfoBox){
    Units::GetUnitBitmap(Units::GetUserUnitByGroup(ugVerticalSpeed),
      &hBitmapUnit, &BitmapUnitPos, &BitmapUnitSize, UNITBITMAPINVERS | UNITBITMAPGRAY);
  } else {
    Units::GetUnitBitmap(Units::GetUserUnitByGroup(ugVerticalSpeed),
      &hBitmapUnit, &BitmapUnitPos, &BitmapUnitSize, UNITBITMAPGRAY);
  }

  xoffset = window.get_width();
  yoffset = window.get_height() / 2;

  window.set_wndproc(GaugeVarioWndProc);
  window.hide();
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
      window.show();
    }
    if (!EnableVarioGauge && lastvisible) {
      window.hide();
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

  hdcDrawWindow.reset();
  hDrawBitMap.reset();
  window.reset();
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
    ValueHeight = (4
		   + Appearance.CDIWindowFont.CapitalHeight
		   + Appearance.TitleWindowFont.CapitalHeight);

    orgMiddle.y = yoffset - ValueHeight/2;
    orgMiddle.x = window.get_right();
    orgTop.y = orgMiddle.y-ValueHeight;
    orgTop.x = window.get_right();
    orgBottom.y = orgMiddle.y + ValueHeight;
    orgBottom.x = window.get_right();

    BitmapCanvas hdcTemp(hdcDrawWindow, hDrawBitMap);
    // copy scale bitmap to memory DC
    if (InfoBoxLayout::dscale>1) {
      if (Appearance.InverseInfoBox)
        hdcDrawWindow.stretch(hdcTemp, 58, 0, 58, 120);
      else
        hdcDrawWindow.stretch(hdcTemp, 0, 0, 58, 120);
    } else {

      if (Appearance.InverseInfoBox)
        hdcDrawWindow.copy(hdcTemp, 58, 0);
      else
        hdcDrawWindow.copy(hdcTemp, 0, 0);

    }

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
      RenderValue(hdcDrawWindow, orgTop.x, orgTop.y, &diValueTop, &diLabelTop,
		  CALCULATED_INFO.NettoAverage30s*LIFTMODIFY, TEXT("NetAvg"));
    } else {
      RenderValue(hdcDrawWindow, orgTop.x, orgTop.y, &diValueTop, &diLabelTop,
		  CALCULATED_INFO.Average30s*LIFTMODIFY, TEXT("Avg"));
    }
  }

  if (Appearance.GaugeVarioMc) {
    double mc = GlidePolar::GetMacCready()*LIFTMODIFY;
    if (CALCULATED_INFO.AutoMacCready)
      RenderValue(hdcDrawWindow, orgBottom.x, orgBottom.y,
		  &diValueBottom, &diLabelBottom,
		  mc, TEXT("Auto Mc"));
    else
      RenderValue(hdcDrawWindow, orgBottom.x, orgBottom.y,
		  &diValueBottom, &diLabelBottom,
		  mc, TEXT("Mc"));
  }

  if (Appearance.GaugeVarioSpeedToFly) {
    RenderSpeedToFly(hdcDrawWindow,
                     window.get_right() - 11, window.get_height() / 2);
  } else {
    RenderClimb(hdcDrawWindow);
  }

  if (Appearance.GaugeVarioBallast) {
    RenderBallast(hdcDrawWindow);
  }

  if (Appearance.GaugeVarioBugs) {
    RenderBugs(hdcDrawWindow);
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
      RenderNeedle(hdcDrawWindow, ival_last, true, true);
    }
    ival_last = ival_av;
  }

  if ((sval != sval_last) || (ival != vval_last)) {
    RenderVarioLine(hdcDrawWindow, vval_last, sval_last, true);
  }
  sval_last = sval;

  if (ival != vval_last) {
    RenderNeedle(hdcDrawWindow, vval_last, false, true);
  }
  vval_last = ival;

  // now draw items
  RenderVarioLine(hdcDrawWindow, ival, sval, false);
  if (Appearance.GaugeVarioAveNeedle) {
    RenderNeedle(hdcDrawWindow, ival_av, true, false);
  }
  RenderNeedle(hdcDrawWindow, ival, false, false);

  if (Appearance.GaugeVarioGross) {
    RenderValue(hdcDrawWindow, orgMiddle.x, orgMiddle.y,
                &diValueMiddle, &diLabelMiddle,
                vvaldisplay,
                TEXT("Gross"));
  }
  RenderZero(hdcDrawWindow);

  window.get_canvas().copy(hdcDrawWindow);

}

void GaugeVario::Repaint(Canvas &canvas){
  canvas.copy(hdcDrawWindow);
}

void GaugeVario::RenderBg(Canvas &canvas) {
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


void GaugeVario::RenderClimb(Canvas &canvas)
{

  int x = window.get_right() - IBLSCALE(14);
  int y = window.get_bottom() - IBLSCALE(24);

  // testing  GPS_INFO.SwitchState.VarioCircling = true;

  if (!dirty) return;

  if (!GPS_INFO.SwitchState.VarioCircling) {
    if (Appearance.InverseInfoBox){
      canvas.black_brush();
      canvas.black_pen();
    } else {
      canvas.white_brush();
      canvas.white_pen();
    }
    canvas.background_opaque();

    canvas.rectangle(x, y, x + IBLSCALE(12), y + IBLSCALE(12));
  } else {
    BitmapCanvas hdcTemp(canvas, hBitmapClimb);
    if (InfoBoxLayout::dscale>1) {
      canvas.stretch(x, y, IBLSCALE(12), IBLSCALE(12), hdcTemp, 12, 0, 12, 12);
    } else {
      canvas.copy(x, y, 12, 12, hdcTemp, 12, 0);
    }
  }
}


void GaugeVario::RenderZero(Canvas &canvas)
{
  static POINT lp[2];
  if (Appearance.InverseInfoBox){
    canvas.white_brush();
    canvas.white_pen();
  } else {
    canvas.black_brush();
    canvas.black_pen();
  }
  canvas.line(0, yoffset, IBLSCALE(17), yoffset);
  canvas.line(0, yoffset + 1, IBLSCALE(17), yoffset + 1);
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


void GaugeVario::RenderVarioLine(Canvas &canvas, int i, int sink, bool clear)
{
  dirty = true;
  if (i==sink) return; // nothing to do

  if (clear){
    canvas.select(blankThickPen);
  } else {
    if (i>sink) {
      canvas.select(blueThickPen);
    } else {
      canvas.select(redThickPen);
    }
  }
  if (i>sink) {
    canvas.polyline(lines + gmax + sink, i - sink);
  } else {
    canvas.polyline(lines + gmax + i, sink - i);
  }
  if (!clear) {
    // clear up naked (sink) edge of polygon, this gives it a nice
    // taper look
    if (Appearance.InverseInfoBox) {
      canvas.black_brush();
      canvas.black_pen();
    } else {
      canvas.white_brush();
      canvas.white_pen();
    }
    canvas.polygon(getPolygon(sink), 3);
  }
}

void GaugeVario::RenderNeedle(Canvas &canvas, int i, bool average, bool clear)
{
  dirty = true;
  bool colorfull = false;

#ifdef FIVV
  colorfull = Appearance.InfoBoxColors;
#endif

  if (clear || !colorfull) {
    // legacy behaviour
    if (clear ^ Appearance.InverseInfoBox) {
      canvas.white_brush();
      canvas.white_pen();
    } else {
      canvas.black_brush();
      canvas.black_pen();
    }
  } else {
    // VENTA2-ADDON Colorful needles
    // code reorganised by JMW
    if (Appearance.InverseInfoBox) {
      if (average) {
	// Average Needle
	if ( i >= 0) {
          canvas.select(greenBrush);
          canvas.white_pen();
	} else {
          canvas.select(redBrush);
          canvas.white_pen();
	}
      } else {
	// varioline needle: b&w as usual, could also change aspect..
        canvas.select(yellowBrush);
        canvas.white_pen();
      }
    } else {
      // Non inverse infoboxes
      if (average) {
	// Average Needle
	if ( i >= 0) {
          canvas.select(greenBrush);
          canvas.black_pen();
	} else {
          canvas.select(redBrush);
          canvas.black_pen();
	}
      } else {
	// varioline needle: b&w as usual, could also change aspect..
        canvas.black_brush();
        canvas.black_pen();
      }
    }
  }

  if (average) {
    canvas.polyline(getPolygon(i), 3);
  } else {
    canvas.polygon(getPolygon(i), 3);
  }
}


// TODO code: Optimise vario rendering, this is slow
void GaugeVario::RenderValue(Canvas &canvas, int x, int y,
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

  canvas.background_transparent();

  if (dirty && (_tcscmp(diLabel->lastText, Label) != 0)) {
    canvas.set_background_color(colTextBackgnd);
    canvas.set_text_color(colTextGray);
    canvas.select(TitleWindowFont);
    tsize = canvas.text_size(Label);
    diLabel->orgText.x = diLabel->recBkg.right - tsize.cx;
    canvas.text_opaque(diLabel->orgText.x, diLabel->orgText.y,
                       &diLabel->recBkg, Label);
    diLabel->recBkg.left = diLabel->orgText.x;
    _tcscpy(diLabel->lastText, Label);
  }

  if (dirty && (diValue->lastValue != Value)) {
    TCHAR Temp[18];
    canvas.set_background_color(colTextBackgnd);
    canvas.set_text_color(colText);
    _stprintf(Temp, TEXT("%.1f"), Value);
    canvas.select(CDIWindowFont);
    tsize = canvas.text_size(Temp);
    diValue->orgText.x = diValue->recBkg.right - tsize.cx;

    canvas.text_opaque(diValue->orgText.x, diValue->orgText.y,
                       &diValue->recBkg, Temp);

    diValue->recBkg.left = diValue->orgText.x;
    diValue->lastValue = Value;
  }

  if (dirty && (diLabel->lastBitMap != hBitmapUnit)) {
    BitmapCanvas hdcTemp(canvas, *hBitmapUnit);
    if (InfoBoxLayout::dscale>1) {
      canvas.stretch(x - IBLSCALE(5), diValue->recBkg.top,
                     IBLSCALE(BitmapUnitSize.x),
                     IBLSCALE(BitmapUnitSize.y),
                     hdcTemp,
                     BitmapUnitPos.x, BitmapUnitPos.y,
                     BitmapUnitSize.x, BitmapUnitSize.y);
    } else {
      canvas.copy(x - 5, diValue->recBkg.top,
                  BitmapUnitSize.x, BitmapUnitSize.y,
                  hdcTemp,
                  BitmapUnitPos.x, BitmapUnitPos.y);
    }
    diLabel->lastBitMap = hBitmapUnit;
  }

}


void GaugeVario::RenderSpeedToFly(Canvas &canvas, int x, int y)
{

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
  int ytop = window.get_top()
    +YOFFSET+nary; // JMW
  int ybottom = window.get_bottom()
    -YOFFSET-nary-InfoBoxLayout::scale; // JMW

  ytop += IBLSCALE(14);
  ybottom -= IBLSCALE(14);
  // JMW
  //  x = rc.left+IBLSCALE(1);
  x = window.get_right() - 2 * ARROWXSIZE;

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
      canvas.black_brush();
      canvas.black_pen();
    } else {
      canvas.white_brush();
      canvas.white_pen();
    }
    canvas.background_opaque();

    // ToDo sgi optimize

    // bottom (too slow)
    canvas.rectangle(x, ybottom + YOFFSET,
                     x + ARROWXSIZE * 2 + 1,
                     ybottom + YOFFSET + nary + ARROWYSIZE +
                     InfoBoxLayout::scale * 2);

    // top (too fast)
    canvas.rectangle(x, ytop - YOFFSET + 1,
                     x + ARROWXSIZE * 2  +1,
                     ytop - YOFFSET - nary + 1 - ARROWYSIZE -
                     InfoBoxLayout::scale * 2);

    RenderClimb(canvas);

    if (Appearance.InverseInfoBox){
      canvas.white_pen();
    } else {
      canvas.black_pen();
    }

    if (Appearance.InfoBoxColors) {
      if (vdiff>0) { // too slow
        canvas.select(redBrush);
        canvas.select(redPen);
      } else {
        canvas.select(blueBrush);
        canvas.select(bluePen);
      }
    } else {
      if (Appearance.InverseInfoBox){
        canvas.white_brush();
      } else {
        canvas.black_brush();
      }
    }

    if (vdiff > 0){ // too slow

      y = ybottom;
      y += YOFFSET;

      while (vdiff > 0){
        if (vdiff > DeltaVstep){
          canvas.rectangle(x, y, x + ARROWXSIZE * 2 + 1, y + ARROWYSIZE - 1);
        } else {
          POINT Arrow[4];
          Arrow[0].x = x; Arrow[0].y = y;
          Arrow[1].x = x+ARROWXSIZE; Arrow[1].y = y+ARROWYSIZE-1;
          Arrow[2].x = x+2*ARROWXSIZE; Arrow[2].y = y;
          Arrow[3].x = x; Arrow[3].y = y;
          canvas.polygon(Arrow, 4);
        }
        vdiff -=DeltaVstep;
        y += ARROWYSIZE;
      }

    } else if (vdiff < 0){ // too fast

      y = ytop;
      y -= YOFFSET;

      while (vdiff < 0){
        if (vdiff < -DeltaVstep){
          canvas.rectangle(x, y + 1,
                           x + ARROWXSIZE * 2 + 1, y - ARROWYSIZE + 2);
        } else {
          POINT Arrow[4];
          Arrow[0].x = x; Arrow[0].y = y;
          Arrow[1].x = x+ARROWXSIZE; Arrow[1].y = y-ARROWYSIZE+1;
          Arrow[2].x = x+2*ARROWXSIZE; Arrow[2].y = y;
          Arrow[3].x = x; Arrow[3].y = y;
          canvas.polygon(Arrow, 4);
        }
        vdiff +=DeltaVstep;
        y -= ARROWYSIZE;
      }

    }

  }

}


void GaugeVario::RenderBallast(Canvas &canvas)
{
  #define TextBal TEXT("Bal")

  static double lastBallast = 1;
  static RECT  recLabelBk = {-1,-1,-1,-1};
  static RECT  recValueBk = {-1,-1,-1,-1};
  static POINT orgLabel  = {-1,-1};
  static POINT orgValue  = {-1,-1};

  if (Appearance.InverseInfoBox){
    canvas.white_pen();
  } else {
    canvas.black_pen();
  }

  if (recLabelBk.left == -1){                               // ontime init, origin and background rect

    SIZE tSize;

    orgLabel.x = 1;                                         // position of ballast label
    orgLabel.y = window.get_top() + 2 +
      (Appearance.TitleWindowFont.CapitalHeight*2)
      - Appearance.TitleWindowFont.AscentHeight;

    orgValue.x = 1;                                         // position of ballast value
    orgValue.y = window.get_top() + 1 +
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

    canvas.select(TitleWindowFont);           // get max label size
    tSize = canvas.text_size(TextBal);

    recLabelBk.right = recLabelBk.left + tSize.cx;          // update back rect with max label size
    recLabelBk.bottom = recLabelBk.top + Appearance.TitleWindowFont.CapitalHeight;

                                                            // get max value size
    tSize = canvas.text_size(TEXT("100%"));

    recValueBk.right = recValueBk.left + tSize.cx;
     // update back rect with max label size
    recValueBk.bottom = recValueBk.top + Appearance.TitleWindowFont.CapitalHeight;

  }

  double BALLAST = GlidePolar::GetBallast();

  if (BALLAST != lastBallast){
       // ballast hase been changed

    TCHAR Temp[18];

    canvas.select(TitleWindowFont);
    canvas.set_background_color(colTextBackgnd);

    if (lastBallast < 0.001 || BALLAST < 0.001){
      if (BALLAST < 0.001)    // new ballast is 0, hide label
        canvas.text_opaque(orgLabel.x, orgLabel.y, &recLabelBk, TEXT(""));
      else {
        canvas.set_text_color(colTextGray);
          // ols ballast was 0, show label
        canvas.text_opaque(orgLabel.x, orgLabel.y, &recLabelBk, TextBal);
      }
    }

    if (BALLAST < 0.001)         // new ballast 0, hide value
      _stprintf(Temp, TEXT(""));
    else
      _stprintf(Temp, TEXT("%.0f%%"), BALLAST*100);

    canvas.set_text_color(colText);
    canvas.text_opaque(orgValue.x, orgValue.y, &recValueBk, Temp);

    lastBallast = BALLAST;

  }

}

void GaugeVario::RenderBugs(Canvas &canvas)
{
  #define TextBug TEXT("Bug")
  static double lastBugs = 1;
  static RECT  recLabelBk = {-1,-1,-1,-1};
  static RECT  recValueBk = {-1,-1,-1,-1};
  static POINT orgLabel  = {-1,-1};
  static POINT orgValue  = {-1,-1};

  if (Appearance.InverseInfoBox){
    canvas.white_pen();
  } else {
    canvas.black_pen();
  }

  if (recLabelBk.left == -1){

    SIZE tSize;

    orgLabel.x = 1;
    orgLabel.y = window.get_bottom()
      - 2 - Appearance.TitleWindowFont.CapitalHeight - Appearance.TitleWindowFont.AscentHeight;

    orgValue.x = 1;
    orgValue.y = window.get_bottom()
      - 1 - Appearance.TitleWindowFont.AscentHeight;

    recLabelBk.left = orgLabel.x;
    recLabelBk.top = orgLabel.y
      + Appearance.TitleWindowFont.AscentHeight - Appearance.TitleWindowFont.CapitalHeight;
    recValueBk.left = orgValue.x;
    recValueBk.top = orgValue.y
      + Appearance.TitleWindowFont.AscentHeight - Appearance.TitleWindowFont.CapitalHeight;

    canvas.select(TitleWindowFont);
    tSize = canvas.text_size(TextBug);

    recLabelBk.right = recLabelBk.left
      + tSize.cx;
    recLabelBk.bottom = recLabelBk.top
      + Appearance.TitleWindowFont.CapitalHeight + Appearance.TitleWindowFont.Height - Appearance.TitleWindowFont.AscentHeight;

    tSize = canvas.text_size(TEXT("100%"));

    recValueBk.right = recValueBk.left + tSize.cx;
    recValueBk.bottom = recValueBk.top + Appearance.TitleWindowFont.CapitalHeight;

  }

  double BUGS = GlidePolar::GetBugs();

  if (BUGS != lastBugs){

    TCHAR Temp[18];

    canvas.select(TitleWindowFont);
    canvas.set_background_color(colTextBackgnd);

    if (lastBugs > 0.999 || BUGS > 0.999){
      if (BUGS > 0.999)
        canvas.text_opaque(orgLabel.x, orgLabel.y, &recLabelBk, TEXT(""));
      else {
        canvas.set_text_color(colTextGray);
        canvas.text_opaque(orgLabel.x, orgLabel.y, &recLabelBk, TextBug);
      }
    }

    if (BUGS > 0.999)
      _stprintf(Temp, TEXT(""));
    else
      _stprintf(Temp, TEXT("%.0f%%"), (1-BUGS)*100);

    canvas.set_text_color(colText);
    canvas.text_opaque(orgLabel.x, orgLabel.y, &recValueBk, Temp);

    lastBugs = BUGS;

  }
}


LRESULT CALLBACK GaugeVarioWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

  switch (uMsg){
    case WM_ERASEBKGND:
      // we don't need one, we just paint over the top
    return TRUE;

    case WM_PAINT:
      if (globalRunningEvent.test() && EnableVarioGauge) {
        PaintCanvas canvas(GaugeVario::window, hwnd);
        GaugeVario::Repaint(canvas);
      }
    break;

  }

  return(DefWindowProc (hwnd, uMsg, wParam, lParam));
}



