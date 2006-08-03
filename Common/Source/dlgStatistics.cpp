
#include "stdafx.h"
#include "XCSoar.h"

#include "WindowControls.h"
#include "Statistics.h"
#include "Externs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "Atmosphere.h"

extern HFONT                                   StatisticsFont;

#define MAXPAGE 7

double Statistics::yscale;
double Statistics::xscale;
double Statistics::y_min;
double Statistics::x_min;
double Statistics::x_max;
double Statistics::y_max;
bool   Statistics::unscaled_x;
bool   Statistics::unscaled_y;

static HPEN penThinSignal = NULL;

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
      yscale = (rc.bottom-rc.top)/yscale;
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
#if (NEWINFOBOX>0)
    SelectObject(hdc, penThinSignal /*GetStockObject(BLACK_PEN)*/);
#else
    SelectObject(hdc, GetStockObject(BLACK_PEN));
#endif
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


void Statistics::DrawLabel(HDC hdc, RECT rc, TCHAR *text, 
			   double xv, double yv) {
  SIZE tsize;
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = (int)((xv-x_min)*xscale)+rc.left-tsize.cx/2;
  int y = (int)((y_max-yv)*yscale)+rc.top-tsize.cy/2;
  SetBkMode(hdc, OPAQUE);
  ExtTextOut(hdc, x, y, ETO_OPAQUE, NULL, text, _tcslen(text), NULL);
  SetBkMode(hdc, TRANSPARENT);

}


void Statistics::DrawXLabel(HDC hdc, RECT rc, TCHAR *text) {
  SIZE tsize;
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = rc.right-tsize.cx;
  int y = rc.bottom-tsize.cy;

  ExtTextOut(hdc, x, y, 0, NULL, text, _tcslen(text), NULL);
}


void Statistics::DrawYLabel(HDC hdc, RECT rc, TCHAR *text) {
  SIZE tsize;
  GetTextExtentPoint(hdc, text, _tcslen(text), &tsize);
  int x = max(2,rc.left-tsize.cx);
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
  if (!ticstep) return;

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

  if (!ticstep) return;

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


void Statistics::RenderBarograph(HDC hdc, RECT rc)
{

  ResetScale();

  if (flightstats.Altitude.sum_n<2) return;

  ScaleXFromData(rc, &flightstats.Altitude);
  ScaleYFromData(rc, &flightstats.Altitude);

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

  if (flightstats.ThermalAverage.sum_n<1) return;
  
  DrawBarChart(hdc, rc,
               &flightstats.ThermalAverage);
  DrawLine(hdc, rc,
           0, MACCREADY, 
           flightstats.ThermalAverage.sum_n+1,
           MACCREADY,
           STYLE_REDTHICK);

  DrawLabel(hdc, rc, TEXT("MC"), 
	    1, MACCREADY);
  
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
  ScaleXFromValue(rc, GlidePolar::Vminsink-2); // GlidePolar::Vminsink);
  ScaleXFromValue(rc, SAFTEYSPEED+2);
  
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


void Statistics::ScaleMakeSquare(RECT rc) {
  if (y_max-y_min<=0) return;
  if (rc.bottom-rc.top<=0) return;
  double ar = ((double)(rc.right-rc.left))/(rc.bottom-rc.top);
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

  yscale = (rc.bottom-rc.top)/(y_max-y_min);
  xscale = (rc.right-rc.left)/(x_max-x_min);
}

#include "OnLineContest.h"
extern OLCOptimizer olc;
static bool olcvalid=false;
static bool olcfinished=false;

void Statistics::RenderTask(HDC hdc, RECT rc, bool olcmode)
{
  int i;

  double lat1 = 0;
  double lon1 = 0;
  double lat2 = 0;
  double lon2 = 0;
  double x1, y1, x2, y2;
  double lat_c, lon_c;
  double aatradius[MAXTASKPOINTS];

  // find center
  ResetScale();

  for (i=0; i<MAXTASKPOINTS; i++) {
    aatradius[i]=0;
  }
  bool nowaypoints = true;
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (Task[i].Index != -1) {
      lat1 = WayPointList[Task[i].Index].Latitude;
      lon1 = WayPointList[Task[i].Index].Longitude;
      ScaleYFromValue(rc, lat1);
      ScaleXFromValue(rc, lon1);
      nowaypoints = false;
    }
  }
  if (nowaypoints && !olcmode) return;

  olc.SetLine();
  int nolc = olc.getN();

  if (olcvalid) {    
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

  for (i=0; i<MAXTASKPOINTS; i++) {
    if (Task[i].Index != -1) {
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

	if (Task[i].AATType == SECTOR) {
	  radius = Task[i].AATSectorRadius;
	} else {
	  radius = Task[i].AATCircleRadius;
	}
	for (int j=0; j<4; j++) {
	  bearing = j*360.0/4;

	  aatlat = FindLatitude(WayPointList[Task[i].Index].Latitude,
				WayPointList[Task[i].Index].Longitude, 
				bearing, radius);
	  aatlon = FindLongitude(WayPointList[Task[i].Index].Latitude,
				 WayPointList[Task[i].Index].Longitude, 
				 bearing, radius);
	  x1 = (aatlon-lon_c)*fastcosine(aatlat);
	  y1 = (aatlat-lat_c);
	  ScaleXFromValue(rc, x1);
	  ScaleYFromValue(rc, y1);
	  if (j==0) {
	    aatradius[i] = fabs(aatlat-WayPointList[Task[i].Index].Latitude);
	  }
	}
      }
    }
  }
  for (i=0; i< nolc; i++) {
    lat1 = olc.getLatitude(i);
    lon1 = olc.getLongitude(i);
    x1 = (lon1-lon_c)*fastcosine(lat1);
    y1 = (lat1-lat_c);
    ScaleXFromValue(rc, x1);
    ScaleYFromValue(rc, y1);
  }

  ScaleMakeSquare(rc);

  // draw aat areas
  if (!olcmode) {
    if (AATEnabled) {
      for (i=MAXTASKPOINTS-1; i>0; i--) {
	if (Task[i].Index != -1) {
	  lat1 = WayPointList[Task[i-1].Index].Latitude;
	  lon1 = WayPointList[Task[i-1].Index].Longitude;
	  lat2 = WayPointList[Task[i].Index].Latitude;
	  lon2 = WayPointList[Task[i].Index].Longitude;
	  x1 = (lon1-lon_c)*fastcosine(lat1);
	  y1 = (lat1-lat_c);
	  x2 = (lon2-lon_c)*fastcosine(lat2);
	  y2 = (lat2-lat_c);
	  
	  SelectObject(hdc, 
		       MapWindow::hAirspaceBrushes[MapWindow::iAirspaceBrush[AATASK]]);
	  SelectObject(hdc, GetStockObject(WHITE_PEN));
	  if (Task[i].AATType == SECTOR) {
	    Segment(hdc,
		    (long)((x2-x_min)*xscale+rc.left),
		    (long)((y_max-y2)*yscale+rc.top),
		    (long)(aatradius[i]*yscale), 
		    rc, 
		    Task[i].AATStartRadial, 
		    Task[i].AATFinishRadial); 
	  } else {
	    Circle(hdc,
		   (long)((x2-x_min)*xscale+rc.left),
		   (long)((y_max-y2)*yscale+rc.top),
		   (long)(aatradius[i]*yscale), 
		   rc);
	  }
	}
      }
    }
  }

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

  if (!olcmode) {
    for (i=MAXTASKPOINTS-1; i>0; i--) {
      if (Task[i].Index != -1) {
	
	lat1 = WayPointList[Task[i-1].Index].Latitude;
	lon1 = WayPointList[Task[i-1].Index].Longitude;
	if (TaskAborted) {
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
	if (Task[i].Index != -1) {
	  if (i<ActiveWayPoint) continue;
	  if (i>=ActiveWayPoint) {
	    if (i==1) {
	      lat1 = WayPointList[Task[i-1].Index].Latitude;
	      lon1 = WayPointList[Task[i-1].Index].Longitude;
	    } else {
	      lat1 = Task[i-1].AATTargetLat;
	      lon1 = Task[i-1].AATTargetLon;
	    }
	    lat2 = Task[i].AATTargetLat;
	    lon2 = Task[i].AATTargetLon;
	  }
	  
	  if (i==ActiveWayPoint) {
	    lat1 = GPS_INFO.Latitude;
	    lon1 = GPS_INFO.Longitude;
	  }
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

  if (olcmode && olcvalid) {
    for (i=0; i< 7-1; i++) {
      switch(OLCRules) {
      case 0:
	lat1 = olc.solution_FAI_sprint.latitude[i];
	lon1 = olc.solution_FAI_sprint.longitude[i];
	lat2 = olc.solution_FAI_sprint.latitude[i+1];
	lon2 = olc.solution_FAI_sprint.longitude[i+1];
	break;
      case 1:
	lat1 = olc.solution_FAI_triangle.latitude[i];
	lon1 = olc.solution_FAI_triangle.longitude[i];
	lat2 = olc.solution_FAI_triangle.latitude[i+1];
	lon2 = olc.solution_FAI_triangle.longitude[i+1];
	break;
      case 2:
	lat1 = olc.solution_FAI_classic.latitude[i];
	lon1 = olc.solution_FAI_classic.longitude[i];
	lat2 = olc.solution_FAI_classic.latitude[i+1];
	lon2 = olc.solution_FAI_classic.longitude[i+1];
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

  ScaleYFromValue(rc, hmin);
  ScaleYFromValue(rc, hmax);
  ScaleXFromValue(rc, tmin);
  ScaleXFromValue(rc, tmax);

  bool labelDry = false;
  bool labelAir = false;
  bool labelDew = false;

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

      if (i> CUSONDE_NUMLEVELS/3) {
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
  int airspace_scansize_i=16;
  int airspace_scansize_j=16;
  double aclat, aclon, ach, acb;
  double fi, fj;
  aclat = GPS_INFO.Latitude;
  aclon = GPS_INFO.Longitude;
  ach = GPS_INFO.Altitude;
  acb = GPS_INFO.TrackBearing;
  double hmin = max(0,GPS_INFO.Altitude-3300);
  double hmax = max(4000,GPS_INFO.Altitude+2000);
  RECT rcd;

  double fh = (ach-hmin)/(hmax-hmin);

  double d_lat[AIRSPACE_SCANSIZE_X];
  double d_lon[AIRSPACE_SCANSIZE_X];
  double d_h[AIRSPACE_SCANSIZE_H];
  int d_airspace[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X];
  int i,j;

  for (j=0; j< AIRSPACE_SCANSIZE_X; j++) { // scan range
    fj = j*1.0/(AIRSPACE_SCANSIZE_X);
    d_lat[j]= FindLatitude(aclat, aclon, acb, range*fj);
    d_lon[j]= FindLongitude(aclat, aclon, acb, range*fj);
  }
  for (i=0; i< AIRSPACE_SCANSIZE_H; i++) { // scan height
    fi = i*1.0/(AIRSPACE_SCANSIZE_H);
    d_h[i] = (hmax-hmin)*fi+hmin;
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

  for (i=0; i< AIRSPACE_SCANSIZE_H; i++) { // scan height
    fi = i*1.0/(AIRSPACE_SCANSIZE_H);
    for (j=0; j< AIRSPACE_SCANSIZE_X; j++) { // scan range
      fj = j*1.0/(AIRSPACE_SCANSIZE_X);

      type = d_airspace[i][j];
      if (type>=0) {
	SelectObject(hdc,
		     MapWindow::hAirspaceBrushes[MapWindow::iAirspaceBrush[type]]);
	SetTextColor(hdc, 
		     MapWindow::Colours[MapWindow::iAirspaceColour[type]]);
	
	rcd.left = iround(fj*(rc.right-rc.left)+rc.left);
	rcd.right = iround(rcd.left+(rc.right-rc.left)/airspace_scansize_j);
	rcd.bottom = iround(fi*(rc.top-rc.bottom)+rc.bottom);
	rcd.top = iround(rcd.bottom+(rc.top-rc.bottom)/airspace_scansize_i);
	
	Rectangle(hdc,rcd.left,rcd.top,rcd.right,rcd.bottom);
	
      }
    }
  }
  //
  POINT line[4];
  int delta;
  SelectObject(hdc, GetStockObject(WHITE_PEN));
  SelectObject(hdc, GetStockObject(WHITE_BRUSH));
  line[0].x = (int)(rc.left+(rc.right-rc.left)/airspace_scansize_j);
  line[0].y = (int)(fh*(rc.top-rc.bottom)+rc.bottom)-1;
  line[1].x = rc.left;
  line[1].y = line[0].y;
  delta = (line[0].x-line[1].x);
  line[2].x = line[1].x;
  line[2].y = line[0].y-delta/2;
  line[3].x = (line[1].x+line[0].x)/2;
  line[3].y = line[0].y;
  Polygon(hdc, line, 4);
  SelectObject(hdc, (HPEN)oldpen);
  DeleteObject(mpen);
}


////////////////////////////////////////////////////////////////


static int page=0;
static WndForm *wf=NULL;
static WndOwnerDrawFrame *wGrid=NULL;
static WndOwnerDrawFrame *wInfo=NULL;
static WndButton *wCalc=NULL;

static void SetCalcCaption(TCHAR* caption) {
  if (wCalc) {
    wCalc->SetCaption(caption);
  }
}


static void OnAnalysisPaint(WindowControl * Sender, HDC hDC){

  RECT  rcgfx;
  HFONT hfOld;

  CopyRect(&rcgfx, Sender->GetBoundRect());

  // background is painted in the base-class

  hfOld = (HFONT)SelectObject(hDC, Sender->GetFont());

  SetBkMode(hDC, TRANSPARENT);
  SetTextColor(hDC, Sender->GetForeColor());

  if (page==0) {
    SetCalcCaption(TEXT("Settings"));
    Statistics::RenderBarograph(hDC, rcgfx);
  }
  if (page==1) {
    SetCalcCaption(TEXT("Task calc"));
    Statistics::RenderClimb(hDC, rcgfx);
  }
  if (page==2) {
    SetCalcCaption(TEXT("Set wind"));
    Statistics::RenderWind(hDC, rcgfx);
  }
  if (page==3) {
    SetCalcCaption(TEXT("Settings"));
    Statistics::RenderGlidePolar(hDC, rcgfx);
  }
  if (page==4) {
    SetCalcCaption(TEXT("Settings"));
    Statistics::RenderTemperature(hDC, rcgfx);
  }
  if (page==5) {
    SetCalcCaption(TEXT("Task calc"));
    Statistics::RenderTask(hDC, rcgfx, false);
  }
  if (page==6) {
    SetCalcCaption(TEXT("Optimise"));
    Statistics::RenderTask(hDC, rcgfx, true);
  }
  if (page==7) {
    SetCalcCaption(TEXT("Nearest"));
    Statistics::RenderAirspace(hDC, rcgfx);
  }

  SelectObject(hDC, hfOld);

}



static void Update(void){
  TCHAR sTmp[1000];
  //  WndProperty *wp;

  switch(page){
    case 0:
      _stprintf(sTmp, TEXT("Analysis: %s"), gettext(TEXT("Barograph")));
      wf->SetCaption(sTmp);

      _stprintf(sTmp, TEXT("%s:\r\n  %.0f-%.0f %s\r\n\r\n%s:\r\n  %.0f %s/hr"),
             gettext(TEXT("Working band")),
             flightstats.Altitude_Base.y_ave*ALTITUDEMODIFY,
             flightstats.Altitude_Ceiling.y_ave*ALTITUDEMODIFY,
             Units::GetAltitudeName(),
             gettext(TEXT("Ceiling trend")),
             flightstats.Altitude_Ceiling.m*ALTITUDEMODIFY,
             Units::GetAltitudeName());

      wInfo->SetCaption(sTmp);

    break;
    case 1:
      _stprintf(sTmp, TEXT("Analysis: %s"), gettext(TEXT("Climb")));
      wf->SetCaption(sTmp);

      _stprintf(sTmp, TEXT("%s:\r\n  %3.1f %s\r\n\r\n%s:\r\n  %3.2f %s"),
             gettext(TEXT("Av climb rate")),
             flightstats.ThermalAverage.y_ave*LIFTMODIFY,
             Units::GetVerticalSpeedName(),
             gettext(TEXT("Climb trend")),
             flightstats.ThermalAverage.m*LIFTMODIFY,
             Units::GetVerticalSpeedName()
             );

      wInfo->SetCaption(sTmp);

    break;
    case 2:
      _stprintf(sTmp, TEXT("Analysis: %s"), gettext(TEXT("Wind at Altitude")));
      wf->SetCaption(sTmp);
      _stprintf(sTmp, TEXT(" "));
      wInfo->SetCaption(sTmp);
    break;
    case 3:
      _stprintf(sTmp, TEXT("Analysis: %s"), gettext(TEXT("Glide Polar")));
      wf->SetCaption(sTmp);
      _stprintf(sTmp, TEXT("%s:\r\n  %3.0f\r\n  at %3.0f %s\r\n\r\n%s:\r\n%3.2f %s\r\n  at %3.0f %s"),
             gettext(TEXT("Best LD")),
             GlidePolar::bestld,
             GlidePolar::Vbestld*SPEEDMODIFY,
             Units::GetHorizontalSpeedName(),
             gettext(TEXT("Min sink")),
             GlidePolar::minsink*LIFTMODIFY,
             Units::GetVerticalSpeedName(),
             GlidePolar::Vminsink*SPEEDMODIFY,
             Units::GetHorizontalSpeedName());

      wInfo->SetCaption(sTmp);
    break;
  case 4:
    _stprintf(sTmp, TEXT("Analysis: %s"), gettext(TEXT("Temp trace")));
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
  case 5:
    _stprintf(sTmp, TEXT("Analysis: %s"), TEXT("Task"));
    wf->SetCaption(sTmp);

    RefreshTaskStatistics();

    TCHAR timetext1[100];
    TCHAR timetext2[100];
    if (AATEnabled) {
      Units::TimeToText(timetext1, (int)CALCULATED_INFO.TaskTimeToGo);
      Units::TimeToText(timetext2, (int)CALCULATED_INFO.AATTimeToGo);

      _stprintf(sTmp, TEXT("Task to go:\r\n  %s\r\nAAT to go:\r\n  %s\r\nDistance to go:\r\n  %5.0f %s\r\nTarget speed:\r\n  %5.0f %s\r\n"),

		timetext1,
		timetext2,
		DISTANCEMODIFY*CALCULATED_INFO.AATTargetDistance,
		Units::GetDistanceName(),
		TASKSPEEDMODIFY*CALCULATED_INFO.AATTargetSpeed,
		Units::GetTaskSpeedName()		
		);
    } else {
      Units::TimeToText(timetext1, (int)CALCULATED_INFO.TaskTimeToGo);
      _stprintf(sTmp, TEXT("Time to go: %s\r\nDistance to go: %5.0f %s\r\n"),
		timetext1,
		DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceToGo,
		Units::GetDistanceName());
    }
    
    wInfo->SetCaption(sTmp);
    break;
  case 6:
    _stprintf(sTmp, TEXT("Analysis: %s"), TEXT("OnLine Contest"));
    wf->SetCaption(sTmp);

    TCHAR sRules[20];
    TCHAR sFinished[20];
    //      TCHAR timetext2[100];
    int dt;
    double d;
    double score;

    olcvalid = false;
    olcfinished = false;
    score = 0;
    switch(OLCRules) {
    case 0:
      _stprintf(sRules,TEXT("Sprint"));
      dt = olc.solution_FAI_sprint.time;
      d = olc.solution_FAI_sprint.distance;
      olcvalid = olc.solution_FAI_sprint.valid;
      score = olc.solution_FAI_sprint.score;
      olcfinished = olc.solution_FAI_sprint.finished;
      break;
    case 1:
      _stprintf(sRules,TEXT("Triangle"));
      dt = olc.solution_FAI_triangle.time;
      d = olc.solution_FAI_triangle.distance;
      olcvalid = olc.solution_FAI_triangle.valid;
      score = olc.solution_FAI_triangle.score;
      olcfinished = olc.solution_FAI_triangle.finished;
      break;
    case 2:
      _stprintf(sRules,TEXT("Classic"));
      dt = olc.solution_FAI_classic.time;
      d = olc.solution_FAI_classic.distance;
      olcvalid = olc.solution_FAI_classic.valid;
      score = olc.solution_FAI_classic.score;
      olcfinished = olc.solution_FAI_classic.finished;
      break;
    }
    if (olcfinished) {
      _tcscpy(sFinished,TEXT(" (Finished)"));
    } else {
      _tcscpy(sFinished,TEXT(" (In progress)"));
    }

    if (olcvalid) {
      TCHAR timetext1[100];
      Units::TimeToText(timetext1, dt);
      _stprintf(sTmp, TEXT("Rules: %s\r\n%s\r\nDistance:\r\n  %5.0f %s\r\nTime: %s\r\nSpeed: %3.0f %s\r\nScore: %.2f\r\n"),
		sRules,
		sFinished,
		DISTANCEMODIFY*d,
		Units::GetDistanceName(),
		timetext1,
		TASKSPEEDMODIFY*d/dt,
		Units::GetTaskSpeedName(),
		score);
    } else {
      _stprintf(sTmp, TEXT("Rules: %s\r\nNo valid path\r\n"),
		sRules);
    }
    wInfo->SetCaption(sTmp);

    break;
  case 7:
    _stprintf(sTmp, TEXT("Analysis: %s"), TEXT("Airspace"));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(TEXT(" "));
    break;
  }

  wGrid->SetVisible(page<8);

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
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){

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

  if (page==0) {
    dlgBasicSettingsShowModal();
  }
  if (page==1) {
#if (NEWINFOBOX>0)
    dlgTaskCalculatorShowModal();
#endif
  }
  if (page==2) {
    dlgWindSettingsShowModal();
  }
  if (page==3) {
    dlgBasicSettingsShowModal();
  }
  if (page==4) {
    dlgBasicSettingsShowModal();
  }
  if (page==5) {
#if (NEWINFOBOX>0)
    dlgTaskCalculatorShowModal();
#endif
  }
  if (page==6) {
    olc.Optimize((CALCULATED_INFO.Flying==1));
  }
  if (page==7) {
#if (NEWAIRSPACEWARNING>0)
    dlgAirspaceWarningShowDlg(true);
#endif
  }
  Update();
}



static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnAnalysisPaint),
  DeclearCallBackEntry(OnNextClicked),
  DeclearCallBackEntry(OnPrevClicked),
  DeclearCallBackEntry(OnCalcClicked),
  DeclearCallBackEntry(NULL)
};

#if (NEWINFOBOX>0)

void dlgAnalysisShowModal(void){

  //  MemCheckPoint();

  wf=NULL;
  wGrid=NULL;
  wInfo=NULL;
  wCalc=NULL;
  olcvalid = false;
  olcfinished = false;
  
  wf = dlgLoadFromXML(CallBackTable, 
		      LocalPathS(TEXT("dlgAnalysis.xml")), 
		      hWndMainWindow,
		      TEXT("IDR_XML_ANALYSIS"));
  if (!wf) return;

  penThinSignal = CreatePen(PS_SOLID, 1 , RGB(50,243,45));

  wf->SetKeyDownNotify(FormKeyDown);

  wGrid = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmGrid"));
  wInfo = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmInfo"));
  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wCalc = ((WndButton *)wf->FindByName(TEXT("cmdCalc")));

  Update();

  wf->ShowModal();

  delete wf;

  //  MemLeakCheck();

  wf = NULL;

  DeleteObject(penThinSignal);

  MapWindow::RequestFastRefresh();
  ClearAirspaceWarnings(false); // airspace warning gets refreshed
  FullScreen();

}

#endif


/////////////
// For backward compatability

#include "InfoBoxLayout.h"


LRESULT CALLBACK AnalysisProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  TCHAR Temp[2048];
  static HDC hdcScreen;
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
          MapWindow::RequestFastRefresh();
          ClearAirspaceWarnings(false); // airspace warning gets refreshed
          FullScreen();
          return TRUE;
        }
      if (LOWORD(wParam) == IDC_ANALYSISNEXT) {
        page++;
        
        // cycle around to start page
        if (page==6) {
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
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, 
		       gettext(TEXT("Wind at Altitude")));

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

        _stprintf(Temp, TEXT("%s: %5.0f %s\r\n%s: %5.0f %s\r\n"),
		  gettext(TEXT("Thermal height")),
		  CuSonde::thermalHeight*ALTITUDEMODIFY,
		  Units::GetAltitudeName(),
		  gettext(TEXT("Cloud base")),
		  CuSonde::cloudBase*ALTITUDEMODIFY,
		  Units::GetAltitudeName());
        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);

        Statistics::RenderTemperature(hdcScreen, rcgfx);

      }
      if (page==5) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, gettext(TEXT("Task")));
	TCHAR timetext1[100];
	TCHAR timetext2[100];
	if (AATEnabled) {
	  Units::TimeToText(timetext1, (int)CALCULATED_INFO.TaskTimeToGo);
	  Units::TimeToText(timetext2, (int)CALCULATED_INFO.AATTimeToGo);
	  _stprintf(Temp, TEXT("Task time to go: %s\r\nAAT time to go: %s\r\nMax dist: %5.0f %s\r\nMin dist: %5.0f %s\r\nMax speed: %5.0f %s\r\nMin speed: %5.0f %s\r\n"),
		    timetext1, 
		    timetext2, 
		    DISTANCEMODIFY*CALCULATED_INFO.AATMaxDistance,
		  Units::GetDistanceName(),
		    DISTANCEMODIFY*CALCULATED_INFO.AATMinDistance,
		  Units::GetDistanceName(),
		    TASKSPEEDMODIFY*CALCULATED_INFO.AATMaxSpeed,
		  Units::GetTaskSpeedName(),
		    TASKSPEEDMODIFY*CALCULATED_INFO.AATMinSpeed,
		  Units::GetTaskSpeedName()
		   );
	} else {
	  Units::TimeToText(timetext1, (int)CALCULATED_INFO.TaskTimeToGo);
	  _stprintf(Temp, TEXT("Time to go: %s\r\nDistance to go: %5.0f %s\r\n"),
		    timetext1,
		    DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceToGo,
		    Units::GetDistanceName());
	}
        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);

        Statistics::RenderTask(hdcScreen, rcgfx, false);

      }
      if (page==6) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, gettext(TEXT("Airspace")));
	_stprintf(Temp, TEXT(""));
        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);
        Statistics::RenderAirspace(hdcScreen, rcgfx);
      }
      SelectObject(hdcScreen, hfOld);
      EndPaint(hDlg, &ps);

      return FALSE;

    case WM_CLOSE:
      MapWindow::RequestFastRefresh();
      ClearAirspaceWarnings(false); // airspace warning gets refreshed
      FullScreen();
    }
  return FALSE;
}

