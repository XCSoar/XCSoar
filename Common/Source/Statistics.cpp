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
#include <math.h>
#include "Statistics.h"
#include "XCSoar.h"
#include "Externs.h"
#include "McReady.h"
#include "Units.h"
#include "InfoBoxLayout.h"
#include "Atmosphere.h"

extern HFONT                                   StatisticsFont;


double Statistics::yscale;
double Statistics::xscale;
double Statistics::y_min;
double Statistics::x_min;
double Statistics::x_max;
double Statistics::y_max;
bool Statistics::unscaled_x;
bool Statistics::unscaled_y;


void Statistics::ResetScale() {
  unscaled_y = true;  
  unscaled_x = true;  
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


  yscale = (y_max - y_min);
  if (yscale>0) {
    yscale = (rc.bottom-rc.top)/yscale;
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
    xscale = (rc.right-rc.left)/xscale;
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
    yscale = (rc.bottom-rc.top)/yscale;
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
    xscale = (rc.right-rc.left)/xscale;
  }

}


void Statistics::StyleLine(HDC hdc, POINT l1, POINT l2,
                           int Style) {
  POINT line[2];
  line[0] = l1;
  line[1] = l2;
  switch (Style) {
  case STYLE_BLUETHIN:
    DrawDashLine(hdc, 1, 
                 l1, 
                 l2, 
                 RGB(0,0,255));
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
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    Polyline(hdc, line, 2);
    break;
  case STYLE_THINDASHPAPER:
    DrawDashLine(hdc, 1, 
                 l1, 
                 l2, 
                 RGB(0xf0,0xf0,0xb0));    
    break;

  default:
    break;
  }

}


void Statistics::DrawXLabel(HDC hdc, RECT rc, TCHAR *text) {
  SIZE tsize;
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = rc.right-tsize.cx+5;
  int y = rc.bottom-tsize.cy;

  ExtTextOut(hdc, x, y, 0, NULL, text, _tcslen(text), NULL);
}


void Statistics::DrawYLabel(HDC hdc, RECT rc, TCHAR *text) {
  SIZE tsize;
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = max(0,rc.left-tsize.cx);
  int y = rc.top;
  ExtTextOut(hdc, x, y, 0, NULL, text, _tcslen(text), NULL);
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
  
  xmin = (int)((xmin-x_min)*xscale)+rc.left;
  xmax = (int)((xmax-x_min)*xscale)+rc.left;
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
  
  xmin = (int)((xmin)*xscale)+rc.left;
  xmax = (int)((xmax)*xscale)+rc.left;
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
  
  xmin = (int)((xmin-x_min)*xscale)+rc.left;
  xmax = (int)((xmax-x_min)*xscale)+rc.left;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  // STYLE_REDTHICK
  StyleLine(hdc, line[0], line[1], Style);

}


void Statistics::DrawBarChart(HDC hdc, RECT rc, LeastSquares* lsdata) {
  int i;

  if (unscaled_x || unscaled_y) {
    return;
  }

  SelectObject(hdc, GetStockObject(BLACK_PEN));
  SelectObject(hdc, GetStockObject(BLACK_BRUSH));

  int xmin, ymin, xmax, ymax;

  for (i=0; i<lsdata->sum_n; i++) {
    xmin = (int)((i+0.2)*xscale)+rc.left;
    ymin = (int)((y_max-y_min)*yscale)+rc.top;
    xmax = (int)((i+0.8)*xscale)+rc.left;
    ymax = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    Rectangle(hdc, 
              xmin, 
              ymin,
              xmax,
              ymax);
  }

}


void Statistics::DrawLineGraph(HDC hdc, RECT rc, LeastSquares* lsdata,
                               int Style) {

  POINT line[2];

  int i;


  int xmin, ymin, xmax, ymax;

  for (i=0; i<lsdata->sum_n-1; i++) {
    xmin = (int)((lsdata->xstore[i]-x_min)*xscale)+rc.left;
    ymin = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    xmax = (int)((lsdata->xstore[i+1]-x_min)*xscale)+rc.left;
    ymax = (int)((y_max-lsdata->ystore[i+1])*yscale)+rc.top;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_DASHGREEN
    // STYLE_MEDIUMBLACK
    StyleLine(hdc, line[0], line[1], Style);
  }
}


void Statistics::DrawXGrid(HDC hdc, RECT rc, double ticstep, double zero,
                           int Style) {

  POINT line[2];

  double xval;

  int xmin, ymin, xmax, ymax;

  for (xval=zero; xval<= x_max; xval+= ticstep) {

    xmin = (int)((xval-x_min)*xscale)+rc.left;
    ymin = rc.top;
    xmax = xmin;
    ymax = rc.bottom;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    StyleLine(hdc, line[0], line[1], Style);
  }

  for (xval=zero; xval>= x_min; xval-= ticstep) {

    xmin = (int)((xval-x_min)*xscale)+rc.left;
    ymin = rc.top;
    xmax = xmin;
    ymax = rc.bottom;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    StyleLine(hdc, line[0], line[1], Style);
  }

}

void Statistics::DrawYGrid(HDC hdc, RECT rc, double ticstep, double zero,
                           int Style) {

  POINT line[2];

  double yval;

  int xmin, ymin, xmax, ymax;

  for (yval=zero; yval<= y_max; yval+= ticstep) {

    xmin = rc.left;
    ymin = (int)((y_max-yval)*yscale)+rc.top;
    xmax = rc.right;
    ymax = ymin;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    StyleLine(hdc, line[0], line[1], Style);
  }

  for (yval=zero; yval>= y_min; yval-= ticstep) {

    xmin = rc.left;
    ymin = (int)((y_max-yval)*yscale)+rc.top;
    xmax = rc.right;
    ymax = ymin;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    StyleLine(hdc, line[0], line[1], Style);
  }
}



/////////////////

void Statistics::Reset() {
  ThermalAverage.Reset();
  Wind_x.Reset();
  Wind_y.Reset();
  Altitude.Reset();
  Altitude_Base.Reset();
  Altitude_Ceiling.Reset();
}


void Statistics::Reset() {
  ThermalAverage.Reset();
  Wind_x.Reset();
  Wind_y.Reset();
  Altitude.Reset();
  Altitude_Base.Reset();
  Altitude_Ceiling.Reset();
}

void Statistics::RenderAirspace(HDC hdc, RECT rc) 
{

}

void Statistics::RenderBarograph(HDC hdc, RECT rc)
{

  ResetScale();
  ScaleXFromData(rc, &flightstats.Altitude);
  ScaleYFromData(rc, &flightstats.Altitude);
  ScaleXFromData(rc, &flightstats.Altitude_Base);
  ScaleYFromData(rc, &flightstats.Altitude_Base);
  ScaleXFromData(rc, &flightstats.Altitude_Ceiling);
  ScaleYFromData(rc, &flightstats.Altitude_Ceiling);

  DrawXGrid(hdc, rc, 
            0.25, flightstats.Altitude.x_min,
            STYLE_THINDASHPAPER);

  DrawYGrid(hdc, rc, 1000/ALTITUDEMODIFY, 0, STYLE_THINDASHPAPER);

  DrawLineGraph(hdc, rc, &flightstats.Altitude,
                STYLE_MEDIUMBLACK);

  DrawTrend(hdc, rc, &flightstats.Altitude_Base, STYLE_BLUETHIN);

  DrawTrend(hdc, rc, &flightstats.Altitude_Ceiling, STYLE_BLUETHIN);

  DrawXLabel(hdc, rc, TEXT("t"));
  DrawYLabel(hdc, rc, TEXT("h"));

}


void Statistics::RenderClimb(HDC hdc, RECT rc) 
{
  ResetScale();
  ScaleYFromData(rc, &flightstats.ThermalAverage);
  ScaleYFromValue(rc, MACCREADY);
  ScaleYFromValue(rc, 0);
  ScaleXFromValue(rc, 0);
  ScaleXFromValue(rc, flightstats.ThermalAverage.sum_n+1);
  
  DrawYGrid(hdc, rc, 
            1.0/LIFTMODIFY, 0,
            STYLE_THINDASHPAPER);
  
  DrawBarChart(hdc, rc,
               &flightstats.ThermalAverage);
  DrawLine(hdc, rc,
           0, MACCREADY, 
           flightstats.ThermalAverage.sum_n+1,
           MACCREADY,
           STYLE_REDTHICK);
  
  DrawTrendN(hdc, rc,
             &flightstats.ThermalAverage,
             STYLE_BLUETHIN);

  DrawXLabel(hdc, rc, TEXT("n"));
  DrawYLabel(hdc, rc, TEXT("w"));

}


void Statistics::RenderGlidePolar(HDC hdc, RECT rc)
{

  ResetScale();
  ScaleYFromValue(rc, 0);
  ScaleYFromValue(rc, GlidePolar::SinkRateFast(0,(int)(SAFTEYSPEED-1)));
  ScaleXFromValue(rc, 0); // GlidePolar::Vminsink);
  ScaleXFromValue(rc, SAFTEYSPEED);
  
  DrawXGrid(hdc, rc, 
            10.0/SPEEDMODIFY, 0,
            STYLE_THINDASHPAPER);
  DrawYGrid(hdc, rc, 
            1.0/LIFTMODIFY, 0,
            STYLE_THINDASHPAPER);
  
  int i;
  double sinkrate0, sinkrate1;
  for (i= GlidePolar::Vminsink; i< SAFTEYSPEED-1;
       i++) {
    
    sinkrate0 = GlidePolar::SinkRateFast(0,i);
    sinkrate1 = GlidePolar::SinkRateFast(0,i+1);
    DrawLine(hdc, rc,
             i, sinkrate0 , 
             i+1, sinkrate1, 
             STYLE_MEDIUMBLACK);
  }
  DrawXLabel(hdc, rc, TEXT("V"));
  DrawYLabel(hdc, rc, TEXT("w"));
}


void Statistics::RenderTemperature(HDC hdc, RECT rc)
{
  ResetScale();

  int i;
  float hmin= 10000;
  float hmax= -10000;
  float tmin= (float)CuSonde::maxGroundTemperature; //RB Type cast fixed
  float tmax= (float)CuSonde::maxGroundTemperature; //RB Type cast fixed

  // find range for scaling of graph
  for (i=0; i<CUSONDE_NUMLEVELS-1; i++) {
    if (CuSonde::cslevels[i].nmeasurements) {

      hmin = min(hmin, i);
      hmax = max(hmax, i);
      tmin = (float)min(tmin, min(CuSonde::cslevels[i].tempDry,
			   min(CuSonde::cslevels[i].airTemp,
			       CuSonde::cslevels[i].dewpoint)));  // RB fix casts
      tmax = (float)max(tmax, max(CuSonde::cslevels[i].tempDry,
			   max(CuSonde::cslevels[i].airTemp,
			       CuSonde::cslevels[i].dewpoint)));  // RB fix casts
    }
  }

  ScaleYFromValue(rc, hmin);
  ScaleYFromValue(rc, hmax);
  ScaleXFromValue(rc, tmin);
  ScaleXFromValue(rc, tmax);

  for (i=0; i<CUSONDE_NUMLEVELS; i++) {

    if (CuSonde::cslevels[i].nmeasurements &&
	CuSonde::cslevels[i+1].nmeasurements) {

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

    }
  }

  DrawXLabel(hdc, rc, TEXT("T°"));
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
  
  if (fabs(flightstats.Altitude_Ceiling.y_max
	   -flightstats.Altitude_Base.y_min)<1) return;

  for (i=0; i<numsteps ; i++) {

    h = (flightstats.Altitude_Ceiling.y_max-flightstats.Altitude_Base.y_min)*
      i/(double)(numsteps-1)+flightstats.Altitude_Base.y_min;

    wind = windanalyser->windstore.getWind(h, &found);
    mag = sqrt(wind.x*wind.x+wind.y*wind.y);

    windstats_mag.least_squares_update(mag, h);

  }
  
  //

  ResetScale();

  ScaleXFromValue(rc, 0);
  ScaleXFromData(rc, &windstats_mag);
  ScaleYFromData(rc, &windstats_mag);

  DrawYGrid(hdc, rc, 1000/ALTITUDEMODIFY, 0, STYLE_THINDASHPAPER);
  DrawXGrid(hdc, rc, 5/LIFTMODIFY, 0, STYLE_THINDASHPAPER);

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

    wind = windanalyser->windstore.getWind(h, &found);
    if (windstats_mag.x_max == 0)
      windstats_mag.x_max=1;  // prevent /0 problems
    wind.x /= windstats_mag.x_max;
    wind.y /= windstats_mag.x_max;
    mag = sqrt(wind.x*wind.x+wind.y*wind.y);
    if (mag<= 0.0) continue;

    angle = atan2(wind.x,-wind.y)*RAD_TO_DEG;

    wv[0].y = (int)((1-hfact)*(rc.bottom-rc.top))+rc.top;
    wv[0].x = (rc.right+rc.left)/2;

    dX = (mag*WINDVECTORMAG);
    dY = 0;
    rotate(&dX,&dY,angle);
    wv[1].x = (int)(wv[0].x + dX);
    wv[1].y = (int)(wv[0].y + dY);
    StyleLine(hdc, wv[0], wv[1], STYLE_MEDIUMBLACK);

    dX = (mag*WINDVECTORMAG-5);
    dY = -3;
    rotate(&dX,&dY,angle);
    wv[2].x = (int)(wv[0].x + dX);
    wv[2].y = (int)(wv[0].y + dY);
    StyleLine(hdc, wv[1], wv[2], STYLE_MEDIUMBLACK);

    dX = (mag*WINDVECTORMAG-5);
    dY = 3;
    rotate(&dX,&dY,angle);
    wv[3].x = (int)(wv[0].x + dX);
    wv[3].y = (int)(wv[0].y + dY);

    StyleLine(hdc, wv[1], wv[3], STYLE_MEDIUMBLACK);

  }

  DrawXLabel(hdc, rc, TEXT("w"));
  DrawYLabel(hdc, rc, TEXT("h"));

}


////////////////

LRESULT CALLBACK AnalysisProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  TCHAR Temp[2048];
  static HDC hdcScreen;
  static int page = 0;
  PAINTSTRUCT ps;
  HDC hdc;
  RECT rc;
  static RECT rcgfx;
  HFONT hfOld;
  HBRUSH background;

  switch (message)
    {
    case WM_INITDIALOG:
                 
      hdcScreen = GetDC(hDlg);
      GetClientRect(hDlg, &rc);

      SendDlgItemMessage(hDlg, IDC_ANALYSISLABEL, WM_SETFONT,
                  (WPARAM)StatisticsFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg, IDC_ANALYSISTEXT, WM_SETFONT,
                  (WPARAM)StatisticsFont,MAKELPARAM(TRUE,0));

      if (!InfoBoxLayout::landscape) {
	rcgfx = rc;
	rcgfx.left  += 10;
	rcgfx.right -= 10;
	rcgfx.top = (rc.bottom-rc.top)*2/10+rc.top;
	rcgfx.bottom = (rc.bottom-rc.top)*2/3+rc.top;
      } else {
	rcgfx = rc;
	rcgfx.left  = long(double(rc.right-rc.left)*0.36)+rc.left;
	rcgfx.right = rc.right-10;
	rcgfx.top = (rc.bottom-rc.top)*2/10+rc.top;
	rcgfx.bottom = rc.bottom;
      }
      
      return TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK) 
        {
          ::ReleaseDC(hDlg, hdcScreen);
          EndDialog(hDlg, LOWORD(wParam));
          MapWindow::RequestFastRefresh= true;
          ClearAirspaceWarnings(false); // airspace warning gets refreshed
          FullScreen();
          return TRUE;
        }
      if (LOWORD(wParam) == IDC_ANALYSISNEXT) {
        page++;
        
        // cycle around to start page
        if (page==5) {
          page=0;
        }
      }
    case WM_PAINT:

      // make background white
      GetClientRect(hDlg, &rc);
      hdc = BeginPaint(hDlg, &ps);

      background = CreateSolidBrush(RGB(0xf0,0xf0,0xb0));
      HGDIOBJ gTemp;
      gTemp = SelectObject(hdcScreen, background);
      SelectObject(hdcScreen, GetStockObject(WHITE_PEN));

      Rectangle(hdcScreen,rcgfx.left,rcgfx.top,rcgfx.right,rcgfx.bottom);
      DeleteObject(background);

      hfOld = (HFONT)SelectObject(hdcScreen, StatisticsFont);
      
      if (page==0) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, gettext(TEXT("Barograph")));

        _stprintf(Temp, TEXT("%s: %5.0f-%5.0f %s\r\n%s: %5.0f %s/hr"),
				 gettext(TEXT("Working band")),
                 flightstats.Altitude_Base.y_ave*ALTITUDEMODIFY, 
                 flightstats.Altitude_Ceiling.y_ave*ALTITUDEMODIFY, 
                 Units::GetAltitudeName(),
				 gettext(TEXT("Ceiling trend")),
                 flightstats.Altitude_Ceiling.m*ALTITUDEMODIFY,
                 Units::GetAltitudeName());

        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);

        Statistics::RenderBarograph(hdcScreen, rcgfx);


      }
      if (page==1) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, gettext(TEXT("Climb")));

        _stprintf(Temp, TEXT("%s: %3.1f %s\r\n%s: %3.2f %s"),                 
				 gettext(TEXT("Average climb rate")),
                  flightstats.ThermalAverage.y_ave*LIFTMODIFY,
                 Units::GetVerticalSpeedName(),
				 gettext(TEXT("Climb trend")),
                 flightstats.ThermalAverage.m*LIFTMODIFY,
                 Units::GetVerticalSpeedName()
                 );

        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);

        Statistics::RenderClimb(hdcScreen, rcgfx);

      }
      if (page==2) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, gettext(TEXT("Wind at Altitude")));

        _stprintf(Temp, TEXT("    "));

        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);

        Statistics::RenderWind(hdcScreen, rcgfx);

      }
      if (page==3) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, gettext(TEXT("Glide Polar")));

        _stprintf(Temp, TEXT("%s: %3.1f %s %3.0f %s\r\n%s: %3.2f %s %s %3.0f %s"),
                 gettext(TEXT("Best LD")),
                 GlidePolar::bestld,
                 gettext(TEXT("at")),
                 GlidePolar::Vbestld*SPEEDMODIFY,
                 Units::GetHorizontalSpeedName(),
                 gettext(TEXT("Min sink")),
                 GlidePolar::minsink*LIFTMODIFY,
                 Units::GetVerticalSpeedName(),
                 gettext(TEXT("at")),
                 GlidePolar::Vminsink*SPEEDMODIFY,
                 Units::GetHorizontalSpeedName());

        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);

        Statistics::RenderGlidePolar(hdcScreen, rcgfx);

      }
      if (page==4) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, gettext(TEXT("Temp trace")));

        _stprintf(Temp, TEXT("%s: %5.0f\r\n%s: %5.0f\r\n"),
		  gettext(TEXT("Thermal height")),
		  CuSonde::thermalHeight*ALTITUDEMODIFY,
		  gettext(TEXT("Cloud base")),
		  CuSonde::cloudBase*ALTITUDEMODIFY);
        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);

        Statistics::RenderTemperature(hdcScreen, rcgfx);

      }

      SelectObject(hdcScreen, hfOld);
      EndPaint(hDlg, &ps);

      return FALSE;

    case WM_CLOSE:
      MapWindow::RequestFastRefresh= true;
      ClearAirspaceWarnings(false); // airspace warning gets refreshed
      FullScreen();
    }
  return FALSE;
}

//
//void Statistics::RenderTemperature(HDC hdc, RECT rc) {
//
//}
//

void Statistics::DrawLabel(HDC hdc, RECT rc, TCHAR *text, 
			   double xv, double yv) {
  SIZE tsize;
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = (int)((xv-x_min)*xscale)+rc.left;//+tsize.cx;
  int y = (int)((y_max-yv)*yscale)+rc.top;//+tsize.cy;
  ExtTextOut(hdc, x, y, 0, NULL, text, _tcslen(text), NULL);

}


void Statistics::ScaleMakeSquare(RECT rc) {
  if (y_max-y_min<=0) return;
  if (rc.bottom-rc.top<=0) return;
  float ar = ((float)(rc.right-rc.left))/(rc.bottom-rc.top);
  float ard = (float)((x_max-x_min)/(y_max-y_min));
  float armod = ard/ar;
  float delta;

  if (armod<1.0) {
    // need to expand width
    delta = (float)((x_max-x_min)*(1.0/armod-1.0)); //RB - fix casts
    x_max += delta/2.0;
    x_min -= delta/2.0;
  } else {
    // need to expand height
    delta = (float)((y_max-y_min)*(armod-1.0));  //RB - fix casts
    y_max += delta/2.0;
    y_min -= delta/2.0;
  }
  // shrink both by 10%
  delta = (float)((x_max-x_min)*(0.1)); //RB - fix cast and remove un-necessary calc
  x_max += delta/2.0;
  x_min -= delta/2.0;
  delta = (float)((y_max-y_min)*(0.1)); //RB - fix cast and remove un-necessary calc
  y_max += delta/2.0;
  y_min -= delta/2.0;

  yscale = (rc.bottom-rc.top)/(y_max-y_min);
  xscale = (rc.right-rc.left)/(x_max-x_min);
}
