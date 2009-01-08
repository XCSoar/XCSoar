/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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


#include "StdAfx.h"
#include "XCSoar.h"
#include "InfoBoxLayout.h"
#include "Statistics.h"
#include "externs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "Atmosphere.h"
#include "RasterTerrain.h"

#define GROUND_COLOUR RGB(157,101,60)

extern HFONT                                   StatisticsFont;

#define MAXPAGE 8

double Statistics::yscale;
double Statistics::xscale;
double Statistics::y_min;
double Statistics::x_min;
double Statistics::x_max;
double Statistics::y_max;
bool   Statistics::unscaled_x;
bool   Statistics::unscaled_y;

static HPEN penThinSignal = NULL;

#define BORDER_X 24
#define BORDER_Y 19

void Statistics::ResetScale() {
  unscaled_y = true;
  unscaled_x = true;
}



void Statistics::Reset() {
  ThermalAverage.Reset();
  Wind_x.Reset();
  Wind_y.Reset();
  Altitude.Reset();
  Altitude_Base.Reset();
  Altitude_Ceiling.Reset();
  Task_Speed.Reset();
  Altitude_Terrain.Reset();
  for(int j=0;j<MAXTASKPOINTS;j++) {
    LegStartTime[j] = -1;
  }
}


void Statistics::ScaleYFromData(RECT rc, LeastSquares* lsdata)
{
  if (!lsdata->sum_n) {
    return;
  }

  if (unscaled_y) {
    y_min = lsdata->y_min;
    y_max = lsdata->y_max;
    unscaled_y = false;
  } else {
    y_min = min(y_min,lsdata->y_min);
    y_max = max(y_max,lsdata->y_max);
  }

  if (lsdata->sum_n>1) {
    double y0, y1;
    y0 = lsdata->x_min*lsdata->m+lsdata->b;
    y1 = lsdata->x_max*lsdata->m+lsdata->b;
    y_min = min(y_min,min(y0,y1));
    y_max = max(y_max,max(y0,y1));
  }


  if (fabs(y_max - y_min) > 50){
    yscale = (y_max - y_min);
    if (yscale>0) {
      yscale = (rc.bottom-rc.top-BORDER_Y)/yscale;
    }
  } else {
    yscale = 2000;
  }
}


void Statistics::ScaleXFromData(RECT rc, LeastSquares* lsdata)
{
  if (!lsdata->sum_n) {
    return;
  }
  if (unscaled_x) {
    x_min = lsdata->x_min;
    x_max = lsdata->x_max;
    unscaled_x = false;
  } else {
    x_min = min(x_min,lsdata->x_min);
    x_max = max(x_max,lsdata->x_max);
  }

  xscale = (x_max-x_min);
  if (xscale>0) {
    xscale = (rc.right-rc.left-BORDER_X)/xscale;
  }
}


void Statistics::ScaleYFromValue(RECT rc, double value)
{
  if (unscaled_y) {
    y_min = value;
    y_max = value;
    unscaled_y = false;
  } else {
    y_min = min(value, y_min);
    y_max = max(value, y_max);
  }

  yscale = (y_max - y_min);
  if (yscale>0) {
    yscale = (rc.bottom-rc.top-BORDER_Y)/yscale;
  }
}


void Statistics::ScaleXFromValue(RECT rc, double value)
{
  if (unscaled_x) {
    x_min = value;
    x_max = value;
    unscaled_x = false;
  } else {
    x_min = min(value, x_min);
    x_max = max(value, x_max);
  }

  xscale = (x_max-x_min);
  if (xscale>0) {
    xscale = (rc.right-rc.left-BORDER_X)/xscale;
  }

}


void Statistics::StyleLine(HDC hdc, POINT l1, POINT l2,
                           int Style) {
  int minwidth = 1;
#ifndef GNAV
  minwidth = 2;
#endif
  POINT line[2];
  line[0] = l1;
  line[1] = l2;
  switch (Style) {
  case STYLE_BLUETHIN:
    DrawDashLine(hdc,
                 minwidth,
                 l1,
                 l2,
                 RGB(0,50,255));
    break;
  case STYLE_REDTHICK:
    DrawDashLine(hdc, 3,
                 l1,
                 l2,
                 RGB(200,50,50));
    break;
  case STYLE_DASHGREEN:
    DrawDashLine(hdc, 2,
                 line[0],
                 line[1],
                 RGB(0,255,0));
    break;
  case STYLE_MEDIUMBLACK:
    SelectObject(hdc, penThinSignal /*GetStockObject(BLACK_PEN)*/);
    Polyline(hdc, line, 2);
    break;
  case STYLE_THINDASHPAPER:
    DrawDashLine(hdc, 1,
                 l1,
                 l2,
                 RGB(0x60,0x60,0x60));
    break;

  default:
    break;
  }

}


void Statistics::DrawLabel(HDC hdc, RECT rc, const TCHAR *text,
			   double xv, double yv) {

  SIZE tsize;
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = (int)((xv-x_min)*xscale)+rc.left-tsize.cx/2+BORDER_X;
  int y = (int)((y_max-yv)*yscale)+rc.top-tsize.cy/2;
  SetBkMode(hdc, OPAQUE);
  ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
  SetBkMode(hdc, TRANSPARENT);
}


void Statistics::DrawNoData(HDC hdc, RECT rc) {

  SIZE tsize;
  TCHAR text[80];
  _stprintf(text,TEXT("%s"), gettext(TEXT("No data")));
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = (int)(rc.left+rc.right-tsize.cx)/2;
  int y = (int)(rc.top+rc.bottom-tsize.cy)/2;
  SetBkMode(hdc, OPAQUE);
  ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
  SetBkMode(hdc, TRANSPARENT);
}


extern HFONT MapLabelFont;


void Statistics::DrawXLabel(HDC hdc, RECT rc, const TCHAR *text) {
  SIZE tsize;
  HFONT hfOld = (HFONT)SelectObject(hdc, MapLabelFont);
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = rc.right-tsize.cx-IBLSCALE(3);
  int y = rc.bottom-tsize.cy;
  ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
  SelectObject(hdc, hfOld);
}


void Statistics::DrawYLabel(HDC hdc, RECT rc, const TCHAR *text) {
  SIZE tsize;
  HFONT hfOld = (HFONT)SelectObject(hdc, MapLabelFont);
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = max(2,rc.left-tsize.cx);
  int y = rc.top;
  ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
  SelectObject(hdc, hfOld);
}


void Statistics::DrawTrend(HDC hdc, RECT rc, LeastSquares* lsdata, int Style)
{
  if (lsdata->sum_n<2) {
    return;
  }

  if (unscaled_x || unscaled_y) {
    return;
  }

  double xmin, xmax, ymin, ymax;
  xmin = lsdata->x_min;
  xmax = lsdata->x_max;
  ymin = lsdata->x_min*lsdata->m+lsdata->b;
  ymax = lsdata->x_max*lsdata->m+lsdata->b;

  xmin = (int)((xmin-x_min)*xscale)+rc.left+BORDER_X;
  xmax = (int)((xmax-x_min)*xscale)+rc.left+BORDER_X;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(hdc, line[0], line[1], Style);

}


void Statistics::DrawTrendN(HDC hdc, RECT rc, LeastSquares* lsdata,
                            int Style)
{
  if (lsdata->sum_n<2) {
    return;
  }

  if (unscaled_x || unscaled_y) {
    return;
  }

  double xmin, xmax, ymin, ymax;
  xmin = 0.5;
  xmax = lsdata->sum_n+0.5;
  ymin = lsdata->x_min*lsdata->m+lsdata->b;
  ymax = lsdata->x_max*lsdata->m+lsdata->b;

  xmin = (int)((xmin)*xscale)+rc.left+BORDER_X;
  xmax = (int)((xmax)*xscale)+rc.left+BORDER_X;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(hdc, line[0], line[1], Style);

}


void Statistics::DrawLine(HDC hdc, RECT rc, double xmin, double ymin,
                          double xmax, double ymax,
                          int Style) {

  if (unscaled_x || unscaled_y) {
    return;
  }

  xmin = (int)((xmin-x_min)*xscale)+rc.left+BORDER_X;
  xmax = (int)((xmax-x_min)*xscale)+rc.left+BORDER_X;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(hdc, line[0], line[1], Style);

}


void Statistics::DrawBarChart(HDC hdc, RECT rc, LeastSquares* lsdata) {
  int i;

  if (unscaled_x || unscaled_y) {
    return;
  }

  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));

  int xmin, ymin, xmax, ymax;

  for (i= 0; i<lsdata->sum_n; i++) {
    xmin = (int)((i+1+0.2)*xscale)+rc.left+BORDER_X;
    ymin = (int)((y_max-y_min)*yscale)+rc.top;
    xmax = (int)((i+1+0.8)*xscale)+rc.left+BORDER_X;
    ymax = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    Rectangle(hdc,
              xmin,
              ymin,
              xmax,
              ymax);
  }

}


void Statistics::DrawFilledLineGraph(HDC hdc, RECT rc, LeastSquares* lsdata,
				     COLORREF color) {

  POINT line[4];

  for (int i=0; i<lsdata->sum_n-1; i++) {
    line[0].x = (int)((lsdata->xstore[i]-x_min)*xscale)+rc.left+BORDER_X;
    line[0].y = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    line[1].x = (int)((lsdata->xstore[i+1]-x_min)*xscale)+rc.left+BORDER_X;
    line[1].y = (int)((y_max-lsdata->ystore[i+1])*yscale)+rc.top;
    line[2].x = line[1].x;
    line[2].y = rc.bottom-BORDER_Y;
    line[3].x = line[0].x;
    line[3].y = rc.bottom-BORDER_Y;
    Polygon(hdc, line, 4);
  }
}



void Statistics::DrawLineGraph(HDC hdc, RECT rc, LeastSquares* lsdata,
                               int Style) {

  POINT line[2];

  for (int i=0; i<lsdata->sum_n-1; i++) {
    line[0].x = (int)((lsdata->xstore[i]-x_min)*xscale)+rc.left+BORDER_X;
    line[0].y = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    line[1].x = (int)((lsdata->xstore[i+1]-x_min)*xscale)+rc.left+BORDER_X;
    line[1].y = (int)((y_max-lsdata->ystore[i+1])*yscale)+rc.top;

    // STYLE_DASHGREEN
    // STYLE_MEDIUMBLACK
    StyleLine(hdc, line[0], line[1], Style);
  }
}


void Statistics::FormatTicText(TCHAR *text, double val, double step) {
  if (step<1.0) {
    _stprintf(text, TEXT("%.1f"), val);
  } else {
    _stprintf(text, TEXT("%.0f"), val);
  }
}


void Statistics::DrawXGrid(HDC hdc, RECT rc, double tic_step, double zero,
                           int Style, double unit_step, bool draw_units) {

  POINT line[2];

  double xval;

  int xmin, ymin, xmax, ymax;
  if (!tic_step) return;

  //  bool do_units = ((x_max-zero)/tic_step)<10;

  for (xval=zero; xval<= x_max; xval+= tic_step) {

    xmin = (int)((xval-x_min)*xscale)+rc.left+BORDER_X;
    ymin = rc.top;
    xmax = xmin;
    ymax = rc.bottom;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax-BORDER_Y;

    // STYLE_THINDASHPAPER
    if ((xval< x_max)
        && (xmin>=rc.left+BORDER_X) && (xmin<=rc.right)) {
      StyleLine(hdc, line[0], line[1], Style);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, xval*unit_step/tic_step, unit_step);
	SetBkMode(hdc, OPAQUE);
	ExtTextOut(hdc, xmin, ymax-IBLSCALE(17),
		   ETO_OPAQUE, NULL, unit_text, _tcslen(unit_text), NULL);
	SetBkMode(hdc, TRANSPARENT);
      }
    }

  }

  for (xval=zero-tic_step; xval>= x_min; xval-= tic_step) {

    xmin = (int)((xval-x_min)*xscale)+rc.left+BORDER_X;
    ymin = rc.top;
    xmax = xmin;
    ymax = rc.bottom;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax-BORDER_Y;

    // STYLE_THINDASHPAPER

    if ((xval> x_min)
        && (xmin>=rc.left+BORDER_X) && (xmin<=rc.right)) {

      StyleLine(hdc, line[0], line[1], Style);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, xval*unit_step/tic_step, unit_step);
	SetBkMode(hdc, OPAQUE);
	ExtTextOut(hdc, xmin, ymax-IBLSCALE(17),
		   ETO_OPAQUE, NULL, unit_text, _tcslen(unit_text), NULL);
	SetBkMode(hdc, TRANSPARENT);
      }
    }

  }

}

void Statistics::DrawYGrid(HDC hdc, RECT rc, double tic_step, double zero,
                           int Style, double unit_step, bool draw_units) {

  POINT line[2];

  double yval;

  int xmin, ymin, xmax, ymax;

  if (!tic_step) return;

  for (yval=zero; yval<= y_max; yval+= tic_step) {

    xmin = rc.left;
    ymin = (int)((y_max-yval)*yscale)+rc.top;
    xmax = rc.right;
    ymax = ymin;
    line[0].x = xmin+BORDER_X;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    if ((yval< y_max) &&
        (ymin>=rc.top) && (ymin<=rc.bottom-BORDER_Y)) {

      StyleLine(hdc, line[0], line[1], Style);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, yval*unit_step/tic_step, unit_step);
	SetBkMode(hdc, OPAQUE);
	ExtTextOut(hdc, xmin+IBLSCALE(8), ymin,
		   ETO_OPAQUE, NULL, unit_text, _tcslen(unit_text), NULL);
	SetBkMode(hdc, TRANSPARENT);
      }
    }
  }

  for (yval=zero-tic_step; yval>= y_min; yval-= tic_step) {

    xmin = rc.left;
    ymin = (int)((y_max-yval)*yscale)+rc.top;
    xmax = rc.right;
    ymax = ymin;
    line[0].x = xmin+BORDER_X;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    if ((yval> y_min) &&
        (ymin>=rc.top) && (ymin<=rc.bottom-BORDER_Y)) {

      StyleLine(hdc, line[0], line[1], Style);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, yval*unit_step/tic_step, unit_step);
	SetBkMode(hdc, OPAQUE);
	ExtTextOut(hdc, xmin+IBLSCALE(8), ymin,
		   ETO_OPAQUE, NULL, unit_text, _tcslen(unit_text), NULL);
	SetBkMode(hdc, TRANSPARENT);
      }
    }
  }
}




/////////////////

#include "OnLineContest.h"
extern OLCOptimizer olc;
static bool olcvalid=false;
static bool olcfinished=false;

void Statistics::RenderBarograph(HDC hdc, RECT rc)
{

  if (flightstats.Altitude.sum_n<2) {
    DrawNoData(hdc, rc);
    return;
  }

  ResetScale();

  ScaleXFromData(rc, &flightstats.Altitude);
  ScaleYFromData(rc, &flightstats.Altitude);
  ScaleYFromValue(rc, 0);
  ScaleXFromValue(rc, flightstats.Altitude.x_min+1.0); // in case no data
  ScaleXFromValue(rc, flightstats.Altitude.x_min);

  LockTaskData();
  for(int j=1;j<MAXTASKPOINTS;j++) {
    if (ValidTaskPoint(j) && (flightstats.LegStartTime[j]>=0)) {
      double xx =
        (flightstats.LegStartTime[j]-CALCULATED_INFO.TakeOffTime)/3600.0;
      if (xx>=0) {
        DrawLine(hdc, rc,
                 xx, y_min,
                 xx, y_max,
                 STYLE_REDTHICK);
      }
    }
  }
  UnlockTaskData();

  HPEN   hpHorizonGround;
  HBRUSH hbHorizonGround;
  hpHorizonGround = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
                                    GROUND_COLOUR);
  hbHorizonGround = (HBRUSH)CreateSolidBrush(GROUND_COLOUR);
  SelectObject(hdc, hpHorizonGround);
  SelectObject(hdc, hbHorizonGround);

  DrawFilledLineGraph(hdc, rc, &flightstats.Altitude_Terrain,
                GROUND_COLOUR);

  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  DeleteObject(hpHorizonGround);
  DeleteObject(hbHorizonGround);

  DrawXGrid(hdc, rc,
            0.5, flightstats.Altitude.x_min,
            STYLE_THINDASHPAPER, 0.5, true);

  DrawYGrid(hdc, rc, 1000/ALTITUDEMODIFY, 0, STYLE_THINDASHPAPER,
            1000, true);

  DrawLineGraph(hdc, rc, &flightstats.Altitude,
                STYLE_MEDIUMBLACK);

  DrawTrend(hdc, rc, &flightstats.Altitude_Base, STYLE_BLUETHIN);

  DrawTrend(hdc, rc, &flightstats.Altitude_Ceiling, STYLE_BLUETHIN);

  DrawXLabel(hdc, rc, TEXT("t"));
  DrawYLabel(hdc, rc, TEXT("h"));

}


void Statistics::RenderSpeed(HDC hdc, RECT rc)
{

  if ((flightstats.Task_Speed.sum_n<2)
      || !ValidTaskPoint(ActiveWayPoint)) {
    DrawNoData(hdc, rc);
    return;
  }

  ResetScale();

  ScaleXFromData(rc, &flightstats.Task_Speed);
  ScaleYFromData(rc, &flightstats.Task_Speed);
  ScaleYFromValue(rc, 0);
  ScaleXFromValue(rc, flightstats.Task_Speed.x_min+1.0); // in case no data
  ScaleXFromValue(rc, flightstats.Task_Speed.x_min);

  LockTaskData();
  for(int j=1;j<MAXTASKPOINTS;j++) {
    if (ValidTaskPoint(j) && (flightstats.LegStartTime[j]>=0)) {
      double xx =
        (flightstats.LegStartTime[j]-CALCULATED_INFO.TaskStartTime)/3600.0;
      if (xx>=0) {
        DrawLine(hdc, rc,
                 xx, y_min,
                 xx, y_max,
                 STYLE_REDTHICK);
      }
    }
  }
  UnlockTaskData();

  DrawXGrid(hdc, rc,
            0.5, flightstats.Task_Speed.x_min,
            STYLE_THINDASHPAPER, 0.5, true);

  DrawYGrid(hdc, rc, 10/TASKSPEEDMODIFY, 0, STYLE_THINDASHPAPER,
            10, true);

  DrawLineGraph(hdc, rc, &flightstats.Task_Speed,
                STYLE_MEDIUMBLACK);

  DrawTrend(hdc, rc, &flightstats.Task_Speed, STYLE_BLUETHIN);

  DrawXLabel(hdc, rc, TEXT("t"));
  DrawYLabel(hdc, rc, TEXT("V"));

}



void Statistics::RenderClimb(HDC hdc, RECT rc)
{

  if (flightstats.ThermalAverage.sum_n<1) {
    DrawNoData(hdc, rc);
    return;
  }

  ResetScale();
  ScaleYFromData(rc, &flightstats.ThermalAverage);
  ScaleYFromValue(rc, (MACCREADY+0.5));
  ScaleYFromValue(rc, 0);

  ScaleXFromValue(rc, -1);
  ScaleXFromValue(rc, flightstats.ThermalAverage.sum_n);

  DrawYGrid(hdc, rc,
            1.0/LIFTMODIFY, 0,
            STYLE_THINDASHPAPER, 1.0, true);

  DrawBarChart(hdc, rc,
               &flightstats.ThermalAverage);

  DrawLine(hdc, rc,
           0, MACCREADY,
           flightstats.ThermalAverage.sum_n,
           MACCREADY,
           STYLE_REDTHICK);

  DrawLabel(hdc, rc, TEXT("MC"),
	    max(0.5, flightstats.ThermalAverage.sum_n-1), MACCREADY);

  DrawTrendN(hdc, rc,
             &flightstats.ThermalAverage,
             STYLE_BLUETHIN);

  DrawXLabel(hdc, rc, TEXT("n"));
  DrawYLabel(hdc, rc, TEXT("w"));

}


void Statistics::RenderGlidePolar(HDC hdc, RECT rc)
{
  int i;

  ResetScale();
  ScaleYFromValue(rc, 0);
  ScaleYFromValue(rc, GlidePolar::SinkRateFast(0,(int)(SAFTEYSPEED-1))*1.1);
  ScaleXFromValue(rc, GlidePolar::Vminsink*0.8);
  ScaleXFromValue(rc, SAFTEYSPEED+2);

  DrawXGrid(hdc, rc,
            10.0/SPEEDMODIFY, 0,
            STYLE_THINDASHPAPER, 10.0, true);
  DrawYGrid(hdc, rc,
            1.0/LIFTMODIFY, 0,
            STYLE_THINDASHPAPER, 1.0, true);

  double sinkrate0, sinkrate1;
  double v0=0, v1;
  bool v0valid = false;
  int i0=0;

  for (i= GlidePolar::Vminsink; i< SAFTEYSPEED-1;
       i++) {

    sinkrate0 = GlidePolar::SinkRateFast(0,i);
    sinkrate1 = GlidePolar::SinkRateFast(0,i+1);
    DrawLine(hdc, rc,
             i, sinkrate0 ,
             i+1, sinkrate1,
             STYLE_MEDIUMBLACK);

    if (CALCULATED_INFO.AverageClimbRateN[i]>0) {
      v1= CALCULATED_INFO.AverageClimbRate[i]
        /CALCULATED_INFO.AverageClimbRateN[i];

      if (v0valid) {

        DrawLine(hdc, rc,
                 i0, v0 ,
                 i, v1,
                 STYLE_DASHGREEN);


      }

      v0 = v1; i0 = i;
      v0valid = true;
    }

  }

  double ff = SAFTEYSPEED/max(1.0, CALCULATED_INFO.VMacCready);
  double sb = GlidePolar::SinkRate(CALCULATED_INFO.VMacCready);
  ff= (sb-MACCREADY)/max(1.0, CALCULATED_INFO.VMacCready);
  DrawLine(hdc, rc,
           0, MACCREADY,
           SAFTEYSPEED,
           MACCREADY+ff*SAFTEYSPEED,
           STYLE_REDTHICK);

  DrawXLabel(hdc, rc, TEXT("V"));
  DrawYLabel(hdc, rc, TEXT("w"));

  TCHAR text[80];
  SetBkMode(hdc, OPAQUE);

  _stprintf(text,TEXT("Weight %.0f kg"),
	    GlidePolar::GetAUW());
  ExtTextOut(hdc, rc.left+IBLSCALE(30),
	     rc.bottom-IBLSCALE(55),
	     ETO_OPAQUE, NULL, text, _tcslen(text), NULL);

  _stprintf(text,TEXT("Wing loading %.1f kg/m2"),
	    GlidePolar::WingLoading);
  ExtTextOut(hdc, rc.left+IBLSCALE(30),
	     rc.bottom-IBLSCALE(40),
	     ETO_OPAQUE, NULL, text, _tcslen(text), NULL);

  SetBkMode(hdc, TRANSPARENT);
}


void Statistics::ScaleMakeSquare(RECT rc) {
  if (y_max-y_min<=0) return;
  if (rc.bottom-rc.top-BORDER_Y<=0) return;
  double ar = ((double)(rc.right-rc.left-BORDER_X))
    /(rc.bottom-rc.top-BORDER_Y);
  double ard = (x_max-x_min)/(y_max-y_min);
  double armod = ard/ar;
  double delta;

  if (armod<1.0) {
    // need to expand width
    delta = (x_max-x_min)*(1.0/armod-1.0);
    x_max += delta/2.0;
    x_min -= delta/2.0;
  } else {
    // need to expand height
    delta = (y_max-y_min)*(armod-1.0);
    y_max += delta/2.0;
    y_min -= delta/2.0;
  }
  // shrink both by 10%
  delta = (x_max-x_min)*(1.1-1.0);
  x_max += delta/2.0;
  x_min -= delta/2.0;
  delta = (y_max-y_min)*(1.1-1.0);
  y_max += delta/2.0;
  y_min -= delta/2.0;

  yscale = (rc.bottom-rc.top-BORDER_Y)/(y_max-y_min);
  xscale = (rc.right-rc.left-BORDER_X)/(x_max-x_min);
}


void Statistics::RenderTask(HDC hdc, RECT rc, bool olcmode)
{
  int i;

  double lat1 = 0;
  double lon1 = 0;
  double lat2 = 0;
  double lon2 = 0;
  double x1, y1, x2=0, y2=0;
  double lat_c, lon_c;
  double aatradius[MAXTASKPOINTS];
  bool olcvalid_this = olcvalid;

  // find center
  ResetScale();

  for (i=0; i<MAXTASKPOINTS; i++) {
    aatradius[i]=0;
  }
  bool nowaypoints = true;
  LockTaskData();
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (ValidTaskPoint(i)) {
      lat1 = WayPointList[Task[i].Index].Latitude;
      lon1 = WayPointList[Task[i].Index].Longitude;
      ScaleYFromValue(rc, lat1);
      ScaleXFromValue(rc, lon1);
      nowaypoints = false;
    }
  }
  UnlockTaskData();
  if (nowaypoints && !olcmode) {
    DrawNoData(hdc, rc);
    return;
  }

  olc.SetLine();
  int nolc = olc.getN();

  if (olcvalid_this) {
    for (i=0; i< nolc; i++) {
      lat1 = olc.getLatitude(i);
      lon1 = olc.getLongitude(i);
      ScaleYFromValue(rc, lat1);
      ScaleXFromValue(rc, lon1);
    }
    if (!olcfinished) {
      lat1 = olc.lat_proj;
      lon1 = olc.lon_proj;
      ScaleYFromValue(rc, lat1);
      ScaleXFromValue(rc, lon1);
    }
  }

  lat_c = (y_max+y_min)/2;
  lon_c = (x_max+x_min)/2;

  int nwps = 0;

  // find scale
  ResetScale();

  lat1 = GPS_INFO.Latitude;
  lon1 = GPS_INFO.Longitude;
  x1 = (lon1-lon_c)*fastcosine(lat1);
  y1 = (lat1-lat_c);
  ScaleXFromValue(rc, x1);
  ScaleYFromValue(rc, y1);

  LockTaskData();
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (ValidTaskPoint(i)) {
      nwps++;
      lat1 = WayPointList[Task[i].Index].Latitude;
      lon1 = WayPointList[Task[i].Index].Longitude;
      x1 = (lon1-lon_c)*fastcosine(lat1);
      y1 = (lat1-lat_c);
      ScaleXFromValue(rc, x1);
      ScaleYFromValue(rc, y1);

      if (AATEnabled) {
	double aatlat;
	double aatlon;
	double bearing;
	double radius;

        if (ValidTaskPoint(i+1)) {
          if (Task[i].AATType == SECTOR) {
            radius = Task[i].AATSectorRadius;
          } else {
            radius = Task[i].AATCircleRadius;
          }
          for (int j=0; j<4; j++) {
            bearing = j*360.0/4;

            FindLatitudeLongitude(WayPointList[Task[i].Index].Latitude,
                                  WayPointList[Task[i].Index].Longitude,
                                  bearing, radius,
                                  &aatlat, &aatlon);
            x1 = (aatlon-lon_c)*fastcosine(aatlat);
            y1 = (aatlat-lat_c);
            ScaleXFromValue(rc, x1);
            ScaleYFromValue(rc, y1);
            if (j==0) {
              aatradius[i] = fabs(aatlat-WayPointList[Task[i].Index].Latitude);
            }
          }
        } else {
          aatradius[i] = 0;
        }
      }
    }
  }
  UnlockTaskData();
  for (i=0; i< nolc; i++) {
    lat1 = olc.getLatitude(i);
    lon1 = olc.getLongitude(i);
    x1 = (lon1-lon_c)*fastcosine(lat1);
    y1 = (lat1-lat_c);
    ScaleXFromValue(rc, x1);
    ScaleYFromValue(rc, y1);
  }

  ScaleMakeSquare(rc);

  DrawXGrid(hdc, rc,
            1.0, 0,
            STYLE_THINDASHPAPER, 1.0, false);
  DrawYGrid(hdc, rc,
            1.0, 0,
            STYLE_THINDASHPAPER, 1.0, false);

  // draw aat areas
  LockTaskData();
  if (!olcmode) {
    if (AATEnabled) {
      for (i=MAXTASKPOINTS-1; i>0; i--) {
	if (ValidTaskPoint(i)) {
	  lat1 = WayPointList[Task[i-1].Index].Latitude;
	  lon1 = WayPointList[Task[i-1].Index].Longitude;
	  lat2 = WayPointList[Task[i].Index].Latitude;
	  lon2 = WayPointList[Task[i].Index].Longitude;
	  x1 = (lon1-lon_c)*fastcosine(lat1);
	  y1 = (lat1-lat_c);
	  x2 = (lon2-lon_c)*fastcosine(lat2);
	  y2 = (lat2-lat_c);

	  SelectObject(hdc,
		       MapWindow::GetAirspaceBrushByClass(AATASK));
	  SelectObject(hdc, GetStockObject(WHITE_PEN));
	  if (Task[i].AATType == SECTOR) {
	    Segment(hdc,
		    (long)((x2-x_min)*xscale+rc.left+BORDER_X),
		    (long)((y_max-y2)*yscale+rc.top),
		    (long)(aatradius[i]*yscale),
		    rc,
		    Task[i].AATStartRadial,
		    Task[i].AATFinishRadial);
	  } else {
	    Circle(hdc,
		   (long)((x2-x_min)*xscale+rc.left+BORDER_X),
		   (long)((y_max-y2)*yscale+rc.top),
		   (long)(aatradius[i]*yscale),
		   rc);
	  }
	}
      }
    }
  }
  UnlockTaskData();

  // draw track

  for (i=0; i< nolc-1; i++) {
    lat1 = olc.getLatitude(i);
    lon1 = olc.getLongitude(i);
    lat2 = olc.getLatitude(i+1);
    lon2 = olc.getLongitude(i+1);
    x1 = (lon1-lon_c)*fastcosine(lat1);
    y1 = (lat1-lat_c);
    x2 = (lon2-lon_c)*fastcosine(lat2);
    y2 = (lat2-lat_c);
    DrawLine(hdc, rc,
	     x1, y1, x2, y2,
	     STYLE_MEDIUMBLACK);
  }

  // draw task lines and labels

  LockTaskData();
  if (!olcmode) {
    for (i=MAXTASKPOINTS-1; i>0; i--) {
      if (ValidTaskPoint(i) && ValidTaskPoint(i-1)) {
        lat1 = WayPointList[Task[i-1].Index].Latitude;
	lon1 = WayPointList[Task[i-1].Index].Longitude;
	if (TaskIsTemporary()) {
	  lat2 = GPS_INFO.Latitude;
	  lon2 = GPS_INFO.Longitude;
	} else {
	  lat2 = WayPointList[Task[i].Index].Latitude;
	  lon2 = WayPointList[Task[i].Index].Longitude;
	}
	x1 = (lon1-lon_c)*fastcosine(lat1);
	y1 = (lat1-lat_c);
	x2 = (lon2-lon_c)*fastcosine(lat2);
	y2 = (lat2-lat_c);

	DrawLine(hdc, rc,
		 x1, y1, x2, y2,
		 STYLE_DASHGREEN);

	TCHAR text[100];
	if ((i==nwps-1) && (Task[i].Index == Task[0].Index)) {
	  _stprintf(text,TEXT("%0d"),1);
	  DrawLabel(hdc, rc, text, x2, y2);
	} else {
	  _stprintf(text,TEXT("%0d"),i+1);
	  DrawLabel(hdc, rc, text, x2, y2);
	}

	if ((i==ActiveWayPoint)&&(!AATEnabled)) {
	  lat1 = GPS_INFO.Latitude;
	  lon1 = GPS_INFO.Longitude;
	  x1 = (lon1-lon_c)*fastcosine(lat1);
	  y1 = (lat1-lat_c);
	  DrawLine(hdc, rc,
		   x1, y1, x2, y2,
		   STYLE_REDTHICK);
	}

      }
    }

    // draw aat task line

    if (AATEnabled) {
      for (i=MAXTASKPOINTS-1; i>0; i--) {
	if (ValidTaskPoint(i) && ValidTaskPoint(i-1)) {
          if (i==1) {
            lat1 = WayPointList[Task[i-1].Index].Latitude;
            lon1 = WayPointList[Task[i-1].Index].Longitude;
          } else {
            lat1 = Task[i-1].AATTargetLat;
            lon1 = Task[i-1].AATTargetLon;
          }
          lat2 = Task[i].AATTargetLat;
          lon2 = Task[i].AATTargetLon;

          /*
	  if (i==ActiveWayPoint) {
	    lat1 = GPS_INFO.Latitude;
	    lon1 = GPS_INFO.Longitude;
	  }
          */

	  x1 = (lon1-lon_c)*fastcosine(lat1);
	  y1 = (lat1-lat_c);
	  x2 = (lon2-lon_c)*fastcosine(lat2);
	  y2 = (lat2-lat_c);

	  DrawLine(hdc, rc,
		   x1, y1, x2, y2,
		   STYLE_REDTHICK);
	}
      }
    }
  }
  UnlockTaskData();

  if (olcmode && olcvalid_this) {
    for (i=0; i< 7-1; i++) {
      switch(OLCRules) {
      case 0:
	lat1 = olc.data.solution_FAI_sprint.latitude[i];
	lon1 = olc.data.solution_FAI_sprint.longitude[i];
	lat2 = olc.data.solution_FAI_sprint.latitude[i+1];
	lon2 = olc.data.solution_FAI_sprint.longitude[i+1];
	break;
      case 1:
	lat1 = olc.data.solution_FAI_triangle.latitude[i];
	lon1 = olc.data.solution_FAI_triangle.longitude[i];
	lat2 = olc.data.solution_FAI_triangle.latitude[i+1];
	lon2 = olc.data.solution_FAI_triangle.longitude[i+1];
	break;
      case 2:
	lat1 = olc.data.solution_FAI_classic.latitude[i];
	lon1 = olc.data.solution_FAI_classic.longitude[i];
	lat2 = olc.data.solution_FAI_classic.latitude[i+1];
	lon2 = olc.data.solution_FAI_classic.longitude[i+1];
	break;
      }
      x1 = (lon1-lon_c)*fastcosine(lat1);
      y1 = (lat1-lat_c);
      x2 = (lon2-lon_c)*fastcosine(lat2);
      y2 = (lat2-lat_c);
      DrawLine(hdc, rc,
	       x1, y1, x2, y2,
	       STYLE_REDTHICK);
    }
    if (!olcfinished) {
      x1 = (olc.lon_proj-lon_c)*fastcosine(lat1);
      y1 = (olc.lat_proj-lat_c);
      DrawLine(hdc, rc,
	       x1, y1, x2, y2,
	       STYLE_BLUETHIN);
    }
  }

  // Draw aircraft on top
  lat1 = GPS_INFO.Latitude;
  lon1 = GPS_INFO.Longitude;
  x1 = (lon1-lon_c)*fastcosine(lat1);
  y1 = (lat1-lat_c);
  DrawLabel(hdc, rc, TEXT("+"), x1, y1);
}


void Statistics::RenderTemperature(HDC hdc, RECT rc)
{
  ResetScale();

  int i;
  float hmin= 10000;
  float hmax= -10000;
  float tmin= (float)CuSonde::maxGroundTemperature;
  float tmax= (float)CuSonde::maxGroundTemperature;

  // find range for scaling of graph
  for (i=0; i<CUSONDE_NUMLEVELS-1; i++) {
    if (CuSonde::cslevels[i].nmeasurements) {

      hmin = min(hmin, i);
      hmax = max(hmax, i);
      tmin = min(tmin, (float)min(CuSonde::cslevels[i].tempDry,
			   (float)min(CuSonde::cslevels[i].airTemp,
			       CuSonde::cslevels[i].dewpoint)));
      tmax = max(tmax, (float)max(CuSonde::cslevels[i].tempDry,
			   (float)max(CuSonde::cslevels[i].airTemp,
			       CuSonde::cslevels[i].dewpoint)));
    }
  }
  if (hmin>= hmax) {
    DrawNoData(hdc, rc);
    return;
  }

  ScaleYFromValue(rc, hmin);
  ScaleYFromValue(rc, hmax);
  ScaleXFromValue(rc, tmin);
  ScaleXFromValue(rc, tmax);

  bool labelDry = false;
  bool labelAir = false;
  bool labelDew = false;

  int ipos = 0;

  for (i=0; i<CUSONDE_NUMLEVELS-1; i++) {

    if (CuSonde::cslevels[i].nmeasurements &&
	CuSonde::cslevels[i+1].nmeasurements) {

      ipos++;

      DrawLine(hdc, rc,
	       CuSonde::cslevels[i].tempDry, i,
	       CuSonde::cslevels[i+1].tempDry, (i+1),
	       STYLE_REDTHICK);

      DrawLine(hdc, rc,
	       CuSonde::cslevels[i].airTemp, i,
	       CuSonde::cslevels[i+1].airTemp, (i+1),
	       STYLE_MEDIUMBLACK);

      DrawLine(hdc, rc,
	       CuSonde::cslevels[i].dewpoint, i,
	       CuSonde::cslevels[i+1].dewpoint, i+1,
	       STYLE_BLUETHIN);

      if (ipos> 2) {
	if (!labelDry) {
	  DrawLabel(hdc, rc, TEXT("DALR"),
		    CuSonde::cslevels[i+1].tempDry, i);
	  labelDry = true;
	} else {
	  if (!labelAir) {
	    DrawLabel(hdc, rc, TEXT("Air"),
		      CuSonde::cslevels[i+1].airTemp, i);
	    labelAir = true;
	  } else {
	    if (!labelDew) {
	      DrawLabel(hdc, rc, TEXT("Dew"),
			CuSonde::cslevels[i+1].dewpoint, i);
	      labelDew = true;
	    }
	  }
	}
      }
    }
  }

  DrawXLabel(hdc, rc, TEXT("T")TEXT(DEG));
  DrawYLabel(hdc, rc, TEXT("h"));
}


// from Calculations.cpp
#include "windanalyser.h"
extern WindAnalyser *windanalyser;

void Statistics::RenderWind(HDC hdc, RECT rc)
{
  int numsteps=10;
  int i;
  double h;
  Vector wind;
  bool found=true;
  double mag;

  LeastSquares windstats_mag;

  if (flightstats.Altitude_Ceiling.y_max
      -flightstats.Altitude_Ceiling.y_min<=10) {
    DrawNoData(hdc, rc);
    return;
  }

  for (i=0; i<numsteps ; i++) {

    h = (flightstats.Altitude_Ceiling.y_max-flightstats.Altitude_Base.y_min)*
      i/(double)(numsteps-1)+flightstats.Altitude_Base.y_min;

    wind = windanalyser->windstore.getWind(GPS_INFO.Time, h, &found);
    mag = sqrt(wind.x*wind.x+wind.y*wind.y);

    windstats_mag.least_squares_update(mag, h);

  }

  //

  ResetScale();

  ScaleXFromData(rc, &windstats_mag);
  ScaleXFromValue(rc, 0);
  ScaleXFromValue(rc, 10.0);

  ScaleYFromData(rc, &windstats_mag);

  DrawXGrid(hdc, rc, 5/SPEEDMODIFY, 0, STYLE_THINDASHPAPER, 5.0, true);
  DrawYGrid(hdc, rc, 1000/ALTITUDEMODIFY, 0, STYLE_THINDASHPAPER,
            1000.0, true);

  DrawLineGraph(hdc, rc, &windstats_mag,
                STYLE_MEDIUMBLACK);

#define WINDVECTORMAG 25

  numsteps = (int)((rc.bottom-rc.top)/WINDVECTORMAG)-1;

  // draw direction vectors

  POINT wv[4];
  double dX, dY;
  double angle;
  double hfact;
  for (i=0; i<numsteps ; i++) {
    hfact = (i+1)/(double)(numsteps+1);
    h = (flightstats.Altitude_Ceiling.y_max-flightstats.Altitude_Base.y_min)*
      hfact+flightstats.Altitude_Base.y_min;

    wind = windanalyser->windstore.getWind(GPS_INFO.Time, h, &found);
    if (windstats_mag.x_max == 0)
      windstats_mag.x_max=1;  // prevent /0 problems
    wind.x /= windstats_mag.x_max;
    wind.y /= windstats_mag.x_max;
    mag = sqrt(wind.x*wind.x+wind.y*wind.y);
    if (mag<= 0.0) continue;

    angle = atan2(wind.x,-wind.y)*RAD_TO_DEG;

    wv[0].y = (int)((1-hfact)*(rc.bottom-rc.top-BORDER_Y))+rc.top;
    wv[0].x = (rc.right+rc.left-BORDER_X)/2+BORDER_X;

    dX = (mag*WINDVECTORMAG);
    dY = 0;
    rotate(dX,dY,angle);
    wv[1].x = (int)(wv[0].x + dX);
    wv[1].y = (int)(wv[0].y + dY);
    StyleLine(hdc, wv[0], wv[1], STYLE_MEDIUMBLACK);

    dX = (mag*WINDVECTORMAG-5);
    dY = -3;
    rotate(dX,dY,angle);
    wv[2].x = (int)(wv[0].x + dX);
    wv[2].y = (int)(wv[0].y + dY);
    StyleLine(hdc, wv[1], wv[2], STYLE_MEDIUMBLACK);

    dX = (mag*WINDVECTORMAG-5);
    dY = 3;
    rotate(dX,dY,angle);
    wv[3].x = (int)(wv[0].x + dX);
    wv[3].y = (int)(wv[0].y + dY);

    StyleLine(hdc, wv[1], wv[3], STYLE_MEDIUMBLACK);

  }

  DrawXLabel(hdc, rc, TEXT("w"));
  DrawYLabel(hdc, rc, TEXT("h"));

}


////////////////////////////////////////////////////////////////


void Statistics::RenderAirspace(HDC hdc, RECT rc) {
  double range = 50.0*1000; // km
  double aclat, aclon, ach, acb;
  double fi, fj;
  aclat = GPS_INFO.Latitude;
  aclon = GPS_INFO.Longitude;
  ach = GPS_INFO.Altitude;
  acb = GPS_INFO.TrackBearing;
  double hmin = max(0,GPS_INFO.Altitude-3300);
  double hmax = max(3300,GPS_INFO.Altitude+1000);
  RECT rcd;

  double d_lat[AIRSPACE_SCANSIZE_X];
  double d_lon[AIRSPACE_SCANSIZE_X];
  double d_alt[AIRSPACE_SCANSIZE_X];
  double d_h[AIRSPACE_SCANSIZE_H];
  int d_airspace[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X];
  int i,j;

  RasterTerrain::Lock();
  // want most accurate rounding here
  RasterTerrain::SetTerrainRounding(0,0);

  for (j=0; j< AIRSPACE_SCANSIZE_X; j++) { // scan range
    fj = j*1.0/(AIRSPACE_SCANSIZE_X-1);
    FindLatitudeLongitude(aclat, aclon, acb, range*fj,
                          &d_lat[j], &d_lon[j]);
    d_alt[j] = RasterTerrain::GetTerrainHeight(d_lat[j],
					       d_lon[j]);
    hmax = max(hmax, d_alt[j]);
  }
  RasterTerrain::Unlock();

  double fh = (ach-hmin)/(hmax-hmin);

  ResetScale();
  ScaleXFromValue(rc, 0);
  ScaleXFromValue(rc, range);
  ScaleYFromValue(rc, hmin);
  ScaleYFromValue(rc, hmax);

  double dfi = 1.0/(AIRSPACE_SCANSIZE_H-1);
  double dfj = 1.0/(AIRSPACE_SCANSIZE_X-1);

  for (i=0; i< AIRSPACE_SCANSIZE_H; i++) { // scan height
    d_h[i] = (hmax-hmin)*i*dfi+hmin;
  }
  for (i=0; i< AIRSPACE_SCANSIZE_H; i++) { // scan height
    for (j=0; j< AIRSPACE_SCANSIZE_X; j++) { // scan range
      d_airspace[i][j]= -1; // no airspace
    }
  }
  ScanAirspaceLine(d_lat, d_lon, d_h, d_airspace);

  int type;

  HPEN mpen = (HPEN)CreatePen(PS_NULL, 0, RGB(0xf0,0xf0,0xb0));
  HPEN oldpen = (HPEN)SelectObject(hdc, (HPEN)mpen);
  double dx = dfj*(rc.right-rc.left-BORDER_X);
  int x0 = rc.left+BORDER_X;
  double dy = dfi*(rc.top-rc.bottom+BORDER_Y);
  int y0 = rc.bottom-BORDER_Y;

  for (i=0; i< AIRSPACE_SCANSIZE_H; i++) { // scan height
    fi = i*dfi;
    for (j=0; j< AIRSPACE_SCANSIZE_X; j++) { // scan range
      fj = j*dfj;

      type = d_airspace[i][j];
      if (type>=0) {
	SelectObject(hdc,
		     MapWindow::GetAirspaceBrushByClass(type));
	SetTextColor(hdc,
		     MapWindow::GetAirspaceColourByClass(type));

	rcd.left = iround((j-0.5)*dx)+x0;
	rcd.right = iround((j+0.5)*dx)+x0;
	rcd.bottom = iround((i+0.5)*dy)+y0;
	rcd.top = iround((i-0.5)*dy)+y0;

	Rectangle(hdc,rcd.left,rcd.top,rcd.right,rcd.bottom);

      }
    }
  }
  // draw ground
  POINT ground[4];
  HPEN   hpHorizonGround;
  HBRUSH hbHorizonGround;
  hpHorizonGround = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
                                    GROUND_COLOUR);
  hbHorizonGround = (HBRUSH)CreateSolidBrush(GROUND_COLOUR);
  SelectObject(hdc, hpHorizonGround);
  SelectObject(hdc, hbHorizonGround);
  for (j=1; j< AIRSPACE_SCANSIZE_X; j++) { // scan range

    ground[0].x = iround((j-1)*dx)+x0;
    ground[1].x = ground[0].x;
    ground[2].x = iround(j*dx)+x0;
    ground[3].x = ground[2].x;
    ground[0].y = y0;
    ground[1].y = iround((d_alt[j-1]-hmin)/(hmax-hmin)
			 *(rc.top-rc.bottom+BORDER_Y))+y0;
    ground[2].y = iround((d_alt[j]-hmin)/(hmax-hmin)
			 *(rc.top-rc.bottom+BORDER_Y))+y0;
    ground[3].y = y0;
    Polygon(hdc, ground, 4);
  }

  //
  POINT line[4];

  if (GPS_INFO.Speed>10.0) {
    double t = range/GPS_INFO.Speed;
    double gfh = (ach+CALCULATED_INFO.Average30s*t-hmin)/(hmax-hmin);
    line[0].x = rc.left;
    line[0].y = (int)(fh*(rc.top-rc.bottom+BORDER_Y)+y0)-1;
    line[1].x = rc.right;
    line[1].y = (int)(gfh*(rc.top-rc.bottom+BORDER_Y)+y0)-1;
    StyleLine(hdc, line[0], line[1], STYLE_BLUETHIN);
  }

  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  SetTextColor(hdc, RGB(0xff,0xff,0xff));

  DrawXGrid(hdc, rc,
            5.0/DISTANCEMODIFY, 0,
            STYLE_THINDASHPAPER, 5.0, true);
  DrawYGrid(hdc, rc, 1000.0/ALTITUDEMODIFY, 0, STYLE_THINDASHPAPER,
            1000.0, true);

  // draw aircraft
  int delta;
  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  line[0].x = (int)(rc.left+(rc.right-rc.left-BORDER_X)/AIRSPACE_SCANSIZE_X-1);
  line[0].y = (int)(fh*(rc.top-rc.bottom+BORDER_Y)+rc.bottom-BORDER_Y)-1;
  line[1].x = rc.left;
  line[1].y = line[0].y;
  delta = (line[0].x-line[1].x);
  line[2].x = line[1].x;
  line[2].y = line[0].y-delta/2;
  line[3].x = (line[1].x+line[0].x)/2;
  line[3].y = line[0].y;
  Polygon(hdc, line, 4);

  DrawXLabel(hdc, rc, TEXT("D"));
  DrawYLabel(hdc, rc, TEXT("h"));

  SelectObject(hdc, (HPEN)oldpen);
  DeleteObject(mpen);
  DeleteObject(hpHorizonGround);
  DeleteObject(hbHorizonGround);
}


////////////////////////////////////////////////////////////////


static int page=0;
static WndForm *wf=NULL;
static WndOwnerDrawFrame *wGrid=NULL;
static WndOwnerDrawFrame *wInfo=NULL;
static WndButton *wCalc=NULL;

static void SetCalcCaption(const TCHAR* caption) {
  if (wCalc) {
    wCalc->SetCaption(gettext(caption));
  }
}


#define ANALYSIS_PAGE_BAROGRAPH    0
#define ANALYSIS_PAGE_CLIMB        1
#define ANALYSIS_PAGE_TASK_SPEED   2
#define ANALYSIS_PAGE_WIND         3
#define ANALYSIS_PAGE_POLAR        4
#define ANALYSIS_PAGE_TEMPTRACE    5
#define ANALYSIS_PAGE_TASK         6
#define ANALYSIS_PAGE_OLC          7
#define ANALYSIS_PAGE_AIRSPACE     8

static void OnAnalysisPaint(WindowControl * Sender, HDC hDC){

  RECT  rcgfx;
  HFONT hfOld;

  CopyRect(&rcgfx, Sender->GetBoundRect());

  // background is painted in the base-class

  hfOld = (HFONT)SelectObject(hDC, Sender->GetFont());

  SetBkMode(hDC, TRANSPARENT);
  SetTextColor(hDC, Sender->GetForeColor());

  switch (page) {
  case ANALYSIS_PAGE_BAROGRAPH:
    SetCalcCaption(TEXT("Settings"));
    Statistics::RenderBarograph(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_CLIMB:
    SetCalcCaption(TEXT("Task calc"));
    Statistics::RenderClimb(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_WIND:
    SetCalcCaption(TEXT("Set wind"));
    Statistics::RenderWind(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_POLAR:
    SetCalcCaption(TEXT("Settings"));
    Statistics::RenderGlidePolar(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_TEMPTRACE:
    SetCalcCaption(TEXT("Settings"));
    Statistics::RenderTemperature(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_TASK:
    SetCalcCaption(TEXT("Task calc"));
    LockTaskData();
    Statistics::RenderTask(hDC, rcgfx, false);
    UnlockTaskData();
    break;
  case ANALYSIS_PAGE_OLC:
    SetCalcCaption(TEXT("Optimise"));
    LockTaskData();
    Statistics::RenderTask(hDC, rcgfx, true);
    UnlockTaskData();
    break;
  case ANALYSIS_PAGE_AIRSPACE:
    SetCalcCaption(TEXT("Warnings"));
    Statistics::RenderAirspace(hDC, rcgfx);
    break;
  case ANALYSIS_PAGE_TASK_SPEED:
    SetCalcCaption(TEXT("Task calc"));
    LockTaskData();
    Statistics::RenderSpeed(hDC, rcgfx);
    UnlockTaskData();
    break;
  default:
    // should never get here!
    break;
  }
  SelectObject(hDC, hfOld);

}



static void Update(void){
  TCHAR sTmp[1000];
  //  WndProperty *wp;
  int dt=1;
  double d=0;

  switch(page){
    case ANALYSIS_PAGE_BAROGRAPH:
      _stprintf(sTmp, TEXT("%s: %s"),
                gettext(TEXT("Analysis")),
                gettext(TEXT("Barograph")));
      wf->SetCaption(sTmp);
      if (flightstats.Altitude_Ceiling.sum_n<2) {
        _stprintf(sTmp, TEXT("\0"));
      } else if (flightstats.Altitude_Ceiling.sum_n<4) {
        _stprintf(sTmp, TEXT("%s:\r\n  %.0f-%.0f %s"),
                  gettext(TEXT("Working band")),
                  flightstats.Altitude_Base.y_ave*ALTITUDEMODIFY,
                  flightstats.Altitude_Ceiling.y_ave*ALTITUDEMODIFY,
                  Units::GetAltitudeName());

      } else {

        _stprintf(sTmp, TEXT("%s:\r\n  %.0f-%.0f %s\r\n\r\n%s:\r\n  %.0f %s/hr"),
                  gettext(TEXT("Working band")),
                  flightstats.Altitude_Base.y_ave*ALTITUDEMODIFY,
                  flightstats.Altitude_Ceiling.y_ave*ALTITUDEMODIFY,
                  Units::GetAltitudeName(),
                  gettext(TEXT("Ceiling trend")),
                  flightstats.Altitude_Ceiling.m*ALTITUDEMODIFY,
                  Units::GetAltitudeName());
      }
      wInfo->SetCaption(sTmp);

    break;
    case ANALYSIS_PAGE_CLIMB:
      _stprintf(sTmp, TEXT("%s: %s"),
                gettext(TEXT("Analysis")),
                gettext(TEXT("Climb")));
      wf->SetCaption(sTmp);

      if (flightstats.ThermalAverage.sum_n==0) {
        _stprintf(sTmp, TEXT("\0"));
      } else if (flightstats.ThermalAverage.sum_n==1) {
        _stprintf(sTmp, TEXT("%s:\r\n  %3.1f %s"),
                  gettext(TEXT("Av climb")),
                  flightstats.ThermalAverage.y_ave*LIFTMODIFY,
                  Units::GetVerticalSpeedName()
                  );
      } else {
        _stprintf(sTmp, TEXT("%s:\r\n  %3.1f %s\r\n\r\n%s:\r\n  %3.2f %s"),
                  gettext(TEXT("Av climb")),
                  flightstats.ThermalAverage.y_ave*LIFTMODIFY,
                  Units::GetVerticalSpeedName(),
                  gettext(TEXT("Climb trend")),
                  flightstats.ThermalAverage.m*LIFTMODIFY,
                  Units::GetVerticalSpeedName()
                  );
      }

      wInfo->SetCaption(sTmp);

    break;
    case ANALYSIS_PAGE_WIND:
      _stprintf(sTmp, TEXT("%s: %s"),
                gettext(TEXT("Analysis")),
                gettext(TEXT("Wind at Altitude")));
      wf->SetCaption(sTmp);
      _stprintf(sTmp, TEXT(" "));
      wInfo->SetCaption(sTmp);
    break;
    case ANALYSIS_PAGE_POLAR:
      _stprintf(sTmp, TEXT("%s: %s (Mass %3.0f kg)"),
                gettext(TEXT("Analysis")),
                gettext(TEXT("Glide Polar")),
                GlidePolar::GetAUW());
      wf->SetCaption(sTmp);
      if (InfoBoxLayout::landscape) {
        _stprintf(sTmp, TEXT("%s:\r\n  %3.0f\r\n  at %3.0f %s\r\n\r\n%s:\r\n%3.2f %s\r\n  at %3.0f %s"),
                  gettext(TEXT("Best LD")),
                  GlidePolar::bestld,
                  GlidePolar::Vbestld*SPEEDMODIFY,
                  Units::GetHorizontalSpeedName(),
                  gettext(TEXT("Min sink")),
                  GlidePolar::minsink*LIFTMODIFY,
                  Units::GetVerticalSpeedName(),
                  GlidePolar::Vminsink*SPEEDMODIFY,
                  Units::GetHorizontalSpeedName()
                  );
      } else {
        _stprintf(sTmp, TEXT("%s:\r\n  %3.0f at %3.0f %s\r\n%s:\r\n  %3.2f %s at %3.0f %s"),
                  gettext(TEXT("Best LD")),
                  GlidePolar::bestld,
                  GlidePolar::Vbestld*SPEEDMODIFY,
                  Units::GetHorizontalSpeedName(),
                  gettext(TEXT("Min sink")),
                  GlidePolar::minsink*LIFTMODIFY,
                  Units::GetVerticalSpeedName(),
                  GlidePolar::Vminsink*SPEEDMODIFY,
                  Units::GetHorizontalSpeedName());
      }

      wInfo->SetCaption(sTmp);
    break;
  case ANALYSIS_PAGE_TEMPTRACE:
    _stprintf(sTmp, TEXT("%s: %s"),
              gettext(TEXT("Analysis")),
              gettext(TEXT("Temp trace")));
    wf->SetCaption(sTmp);

    _stprintf(sTmp, TEXT("%s:\r\n  %5.0f %s\r\n\r\n%s:\r\n  %5.0f %s\r\n"),
	      gettext(TEXT("Thermal height")),
	      CuSonde::thermalHeight*ALTITUDEMODIFY,
	      Units::GetAltitudeName(),
	      gettext(TEXT("Cloud base")),
	      CuSonde::cloudBase*ALTITUDEMODIFY,
	      Units::GetAltitudeName());

    wInfo->SetCaption(sTmp);
    break;
  case ANALYSIS_PAGE_TASK_SPEED:
    _stprintf(sTmp, TEXT("%s: %s"),
              gettext(TEXT("Analysis")),
              gettext(TEXT("Task speed")));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(TEXT(""));
    break;
  case ANALYSIS_PAGE_TASK:
    _stprintf(sTmp, TEXT("%s: %s"),
              gettext(TEXT("Analysis")),
              gettext(TEXT("Task")));
    wf->SetCaption(sTmp);

    RefreshTaskStatistics();

    if (!ValidTaskPoint(ActiveWayPoint)) {
      _stprintf(sTmp, gettext(TEXT("No task")));
    } else {
      TCHAR timetext1[100];
      TCHAR timetext2[100];
      if (AATEnabled) {
        Units::TimeToText(timetext1, (int)CALCULATED_INFO.TaskTimeToGo);
        Units::TimeToText(timetext2, (int)CALCULATED_INFO.AATTimeToGo);

        if (InfoBoxLayout::landscape) {
          _stprintf(sTmp,
                    TEXT("%s:\r\n  %s\r\n%s:\r\n  %s\r\n%s:\r\n  %5.0f %s\r\n%s:\r\n  %5.0f %s\r\n"),
                    gettext(TEXT("Task to go")),
                    timetext1,
                    gettext(TEXT("AAT to go")),
                    timetext2,
                    gettext(TEXT("Distance to go")),
                    DISTANCEMODIFY*CALCULATED_INFO.AATTargetDistance,
                    Units::GetDistanceName(),
                    gettext(TEXT("Target speed")),
                    TASKSPEEDMODIFY*CALCULATED_INFO.AATTargetSpeed,
                    Units::GetTaskSpeedName()
                    );
        } else {
          _stprintf(sTmp,
                    TEXT("%s: %s\r\n%s: %s\r\n%s: %5.0f %s\r\n%s: %5.0f %s\r\n"),
                    gettext(TEXT("Task to go")),
                    timetext1,
                    gettext(TEXT("AAT to go")),
                    timetext2,
                    gettext(TEXT("Distance to go")),
                    DISTANCEMODIFY*CALCULATED_INFO.AATTargetDistance,
                    Units::GetDistanceName(),
                    gettext(TEXT("Target speed")),
                    TASKSPEEDMODIFY*CALCULATED_INFO.AATTargetSpeed,
                    Units::GetTaskSpeedName()
                    );
        }
      } else {
        Units::TimeToText(timetext1, (int)CALCULATED_INFO.TaskTimeToGo);
        _stprintf(sTmp, TEXT("%s: %s\r\n%s: %5.0f %s\r\n"),
                  gettext(TEXT("Task to go")),
                  timetext1,
                  gettext(TEXT("Distance to go")),
                  DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceToGo,
                  Units::GetDistanceName());
      }
    }
    wInfo->SetCaption(sTmp);
    break;
  case ANALYSIS_PAGE_OLC:
    _stprintf(sTmp, TEXT("%s: %s"),
              gettext(TEXT("Analysis")),
              gettext(TEXT("OnLine Contest")));
    wf->SetCaption(sTmp);

    TCHAR sFinished[20];
    double score;

    switch(OLCRules) {
    case 0:
      dt = olc.data.solution_FAI_sprint.time;
      d = olc.data.solution_FAI_sprint.distance;
      olcvalid = olc.data.solution_FAI_sprint.valid;
      score = olc.data.solution_FAI_sprint.score;
      olcfinished = olc.data.solution_FAI_sprint.finished;
      break;
    case 1:
      dt = olc.data.solution_FAI_triangle.time;
      d = olc.data.solution_FAI_triangle.distance;
      olcvalid = olc.data.solution_FAI_triangle.valid;
      score = olc.data.solution_FAI_triangle.score;
      olcfinished = olc.data.solution_FAI_triangle.finished;
      break;
    case 2:
      dt = olc.data.solution_FAI_classic.time;
      d = olc.data.solution_FAI_classic.distance;
      olcvalid = olc.data.solution_FAI_classic.valid;
      score = olc.data.solution_FAI_classic.score;
      olcfinished = olc.data.solution_FAI_classic.finished;
      break;
    default:
      olcvalid = false;
      olcfinished = false;
      score = 0;
      d = 0;
      dt = 0;
    }
    if (olcfinished) {
      _tcscpy(sFinished,TEXT("Finished"));
    } else {
      _tcscpy(sFinished,TEXT("..."));
    }

    if (olcvalid) {
      TCHAR timetext1[100];
      Units::TimeToText(timetext1, dt);
      if (InfoBoxLayout::landscape) {
        _stprintf(sTmp,
                  TEXT("%s\r\n%s:\r\n  %5.0f %s\r\n%s: %s\r\n%s: %3.0f %s\r\n%s: %.2f\r\n"),
                  sFinished,
                  gettext(TEXT("Distance")),
                  DISTANCEMODIFY*d,
                  Units::GetDistanceName(),
                  gettext(TEXT("Time")),
                  timetext1,
                  gettext(TEXT("Speed")),
                  TASKSPEEDMODIFY*d/dt,
                  Units::GetTaskSpeedName(),
                  gettext(TEXT("Score")),
                  score);
      } else {
        _stprintf(sTmp,
                  TEXT("%s\r\n%s: %5.0f %s\r\n%s: %s\r\n%s: %3.0f %s\r\n%s: %.2f\r\n"),
                  sFinished,
                  gettext(TEXT("Distance")),
                  DISTANCEMODIFY*d,
                  Units::GetDistanceName(),
                  gettext(TEXT("Time")),
                  timetext1,
                  gettext(TEXT("Speed")),
                  TASKSPEEDMODIFY*d/dt,
                  Units::GetTaskSpeedName(),
                  gettext(TEXT("Score")),
                  score);
      }
    } else {
      _stprintf(sTmp, TEXT("%s\r\n"),
                gettext(TEXT("No valid path")));
    }
    wInfo->SetCaption(sTmp);

    break;
  case ANALYSIS_PAGE_AIRSPACE:
    _stprintf(sTmp, TEXT("%s: %s"),
              gettext(TEXT("Analysis")),
              gettext(TEXT("Airspace")));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(TEXT(" "));
    break;
  }

  wGrid->SetVisible(page<MAXPAGE+1);

  if (wGrid != NULL)
    wGrid->Redraw();

}

static void NextPage(int Step){
  page += Step;
  if (page > MAXPAGE)
    page = 0;
  if (page < 0)
    page = MAXPAGE;
  Update();
}


static void OnNextClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  (void)Sender; (void)lParam;

  if (wGrid->GetFocused())
    return(0);

  switch(wParam & 0xffff){
    case VK_LEFT:
    case '6':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdPrev")))->GetHandle());
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
    case VK_RIGHT:
    case '7':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdNext")))->GetHandle());
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}

static void OnCalcClicked(WindowControl * Sender,
			  WndListFrame::ListInfo_t *ListInfo){
  (void)ListInfo;
  (void)Sender;
  if (page==ANALYSIS_PAGE_BAROGRAPH) {
    dlgBasicSettingsShowModal();
  }
  if (page==ANALYSIS_PAGE_CLIMB) {
    wf->SetVisible(false);
    dlgTaskCalculatorShowModal();
    wf->SetVisible(true);
  }
  if (page==ANALYSIS_PAGE_WIND) {
    dlgWindSettingsShowModal();
  }
  if (page==ANALYSIS_PAGE_POLAR) {
    dlgBasicSettingsShowModal();
  }
  if (page==ANALYSIS_PAGE_TEMPTRACE) {
    dlgBasicSettingsShowModal();
  }
  if ((page==ANALYSIS_PAGE_TASK) || (page==ANALYSIS_PAGE_TASK_SPEED)) {
    wf->SetVisible(false);
    dlgTaskCalculatorShowModal();
    wf->SetVisible(true);
  }
  if (page==ANALYSIS_PAGE_OLC) {
    StartHourglassCursor();
    olc.Optimize((CALCULATED_INFO.Flying==1));
    StopHourglassCursor();
  }
  if (page==ANALYSIS_PAGE_AIRSPACE) {
    dlgAirspaceWarningShowDlg(true);
  }
  Update();
}



static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAnalysisPaint),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnCalcClicked),
  DeclareCallBackEntry(NULL)
};


void dlgAnalysisShowModal(void){

  wf=NULL;
  wGrid=NULL;
  wInfo=NULL;
  wCalc=NULL;
  olcvalid = false;
  olcfinished = false;

  if (!InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgAnalysis_L.xml"));
    wf = dlgLoadFromXML(CallBackTable,

                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_ANALYSIS_L"));
  } else  {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgAnalysis.xml"));
    wf = dlgLoadFromXML(CallBackTable,
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_ANALYSIS"));
  }

  if (!wf) return;

#ifndef GNAV
  penThinSignal = CreatePen(PS_SOLID, 2 , RGB(50,243,45));
#else
  penThinSignal = CreatePen(PS_SOLID, 1 , RGB(50,243,45));
#endif

  wf->SetKeyDownNotify(FormKeyDown);

  wGrid = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmGrid"));
  wInfo = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmInfo"));
  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wCalc = ((WndButton *)wf->FindByName(TEXT("cmdCalc")));

  Update();

  wf->ShowModal();

  delete wf;

  wf = NULL;

  DeleteObject(penThinSignal);

  MapWindow::RequestFastRefresh();
  ClearAirspaceWarnings(false); // airspace warning gets refreshed
  FullScreen();

}


