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

#include "FlightStatistics.hpp"
#include "XCSoar.h"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/MainWindow.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "InfoBoxLayout.h"
#include "RasterTerrain.h"
#include "Blackboard.hpp"
#include "Language.hpp"
#include "McReady.h"
#include "GlideComputer.hpp"
#include "Atmosphere.h"
#include "Protection.hpp"
#include "SettingsTask.hpp"
#include "SettingsComputer.hpp"
#include "Task.h"
#include "WayPoint.hpp"


void FlightStatistics::Reset() {
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

#include "Screen/Chart.hpp"

void FlightStatistics::RenderBarograph(Canvas &canvas, const RECT rc)
{
  Chart chart(canvas, rc);
  ScopeLock protect(mutexTaskData);

  if (Altitude.sum_n<2) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleXFromData(&Altitude);
  chart.ScaleYFromData(&Altitude);
  chart.ScaleYFromValue(0);
  chart.ScaleXFromValue(Altitude.x_min+1.0); // in case no data
  chart.ScaleXFromValue(Altitude.x_min);

  for(int j=1;j<MAXTASKPOINTS;j++) {
    if (ValidTaskPoint(j) && (LegStartTime[j]>=0)) {
      double xx =
        (LegStartTime[j]-CALCULATED_INFO.TakeOffTime)/3600.0;
      if (xx>=0) {
        chart.DrawLine(xx, chart.getYmin(),
		       xx, chart.getYmax(),
		       Chart::STYLE_REDTHICK);
      }
    }
  }

  HPEN   hpHorizonGround;
  HBRUSH hbHorizonGround;
  hpHorizonGround = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
                                    Chart::GROUND_COLOUR);
  hbHorizonGround = (HBRUSH)CreateSolidBrush(Chart::GROUND_COLOUR);
  SelectObject(canvas, hpHorizonGround);
  SelectObject(canvas, hbHorizonGround);

  chart.DrawFilledLineGraph(&Altitude_Terrain, Chart::GROUND_COLOUR);
  canvas.white_pen();
  canvas.white_brush();

  DeleteObject(hpHorizonGround);
  DeleteObject(hbHorizonGround);

  chart.DrawXGrid(0.5, Altitude.x_min, Chart::STYLE_THINDASHPAPER, 0.5, true);
  chart.DrawYGrid(1000/ALTITUDEMODIFY, 0, Chart::STYLE_THINDASHPAPER, 1000, true);
  chart.DrawLineGraph(&Altitude, Chart::STYLE_MEDIUMBLACK);

  chart.DrawTrend(&Altitude_Base, Chart::STYLE_BLUETHIN);
  chart.DrawTrend(&Altitude_Ceiling, Chart::STYLE_BLUETHIN);

  chart.DrawXLabel(TEXT("t"));
  chart.DrawYLabel(TEXT("h"));
}


void FlightStatistics::RenderSpeed(Canvas &canvas, const RECT rc)
{
  Chart chart(canvas, rc);
  ScopeLock protect(mutexTaskData);

  if ((Task_Speed.sum_n<2)
      || !ValidTaskPoint(ActiveWayPoint)) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleXFromData(&Task_Speed);
  chart.ScaleYFromData( &Task_Speed);
  chart.ScaleYFromValue( 0);
  chart.ScaleXFromValue(Task_Speed.x_min+1.0); // in case no data
  chart.ScaleXFromValue(Task_Speed.x_min);

  for(int j=1;j<MAXTASKPOINTS;j++) {
    if (ValidTaskPoint(j) && (LegStartTime[j]>=0)) {
      double xx =
        (LegStartTime[j]-CALCULATED_INFO.TaskStartTime)/3600.0;
      if (xx>=0) {
        chart.DrawLine(xx, chart.getYmin(),
		       xx, chart.getYmax(),
		       Chart::STYLE_REDTHICK);
      }
    }
  }

  chart.DrawXGrid(0.5, Task_Speed.x_min,
		  Chart::STYLE_THINDASHPAPER, 0.5, true);
  chart.DrawYGrid(10/TASKSPEEDMODIFY, 0, Chart::STYLE_THINDASHPAPER,
		  10, true);
  chart.DrawLineGraph(&Task_Speed, Chart::STYLE_MEDIUMBLACK);
  chart.DrawTrend(&Task_Speed, Chart::STYLE_BLUETHIN);

  chart.DrawXLabel(TEXT("t"));
  chart.DrawYLabel(TEXT("V"));

}



void FlightStatistics::RenderClimb(Canvas &canvas, const RECT rc)
{
  Chart chart(canvas, rc);

  if (ThermalAverage.sum_n<1) {
    chart.DrawNoData();
    return;
  }
  double MACCREADY = GlidePolar::GetMacCready();

  chart.ScaleYFromData( &ThermalAverage);
  chart.ScaleYFromValue( (MACCREADY+0.5));
  chart.ScaleYFromValue( 0);

  chart.ScaleXFromValue(-1);
  chart.ScaleXFromValue(ThermalAverage.sum_n);

  chart.DrawYGrid(1.0/LIFTMODIFY, 0, Chart::STYLE_THINDASHPAPER, 1.0, true);
  chart.DrawBarChart(&ThermalAverage);

  chart.DrawLine(0, MACCREADY, ThermalAverage.sum_n,
		 MACCREADY, Chart::STYLE_REDTHICK);

  chart.DrawLabel(TEXT("MC"), max(0.5, ThermalAverage.sum_n-1), MACCREADY);

  chart.DrawTrendN(&ThermalAverage, Chart::STYLE_BLUETHIN);

  chart.DrawXLabel(TEXT("n"));
  chart.DrawYLabel(TEXT("w"));
}


void FlightStatistics::RenderGlidePolar(Canvas &canvas, const RECT rc)
{
  int i;
  Chart chart(canvas, rc);
  ScopeLock protect(mutexFlightData);

  chart.ScaleYFromValue( 0);
  chart.ScaleYFromValue( GlidePolar::SinkRateFast(0,(int)(SAFTEYSPEED-1))*1.1);
  chart.ScaleXFromValue(GlidePolar::Vminsink*0.8);
  chart.ScaleXFromValue(SAFTEYSPEED+2);

  chart.DrawXGrid(10.0/SPEEDMODIFY, 0,
		  Chart::STYLE_THINDASHPAPER, 10.0, true);
  chart.DrawYGrid(1.0/LIFTMODIFY, 0,
		  Chart::STYLE_THINDASHPAPER, 1.0, true);

  double sinkrate0, sinkrate1;
  double v0=0, v1;
  bool v0valid = false;
  int i0=0;

  for (i= GlidePolar::Vminsink; i< SAFTEYSPEED-1;
       i++) {

    sinkrate0 = GlidePolar::SinkRateFast(0,i);
    sinkrate1 = GlidePolar::SinkRateFast(0,i+1);
    chart.DrawLine(i, sinkrate0 ,
		   i+1, sinkrate1,
		   Chart::STYLE_MEDIUMBLACK);

    if (CALCULATED_INFO.AverageClimbRateN[i]>0) {
      v1= CALCULATED_INFO.AverageClimbRate[i]
        /CALCULATED_INFO.AverageClimbRateN[i];

      if (v0valid) {
        chart.DrawLine(i0, v0 ,
		       i, v1,
		       Chart::STYLE_DASHGREEN);
      }

      v0 = v1; i0 = i;
      v0valid = true;
    }
  }

  double MACCREADY = GlidePolar::GetMacCready();

  double ff = SAFTEYSPEED/max(1.0, CALCULATED_INFO.VMacCready);
  double sb = GlidePolar::SinkRate(CALCULATED_INFO.VMacCready);
  ff= (sb-MACCREADY)/max(1.0, CALCULATED_INFO.VMacCready);

  chart.DrawLine(0, MACCREADY,
		 SAFTEYSPEED, MACCREADY+ff*SAFTEYSPEED,
		 Chart::STYLE_REDTHICK);

  chart.DrawXLabel(TEXT("V"));
  chart.DrawYLabel(TEXT("w"));

  TCHAR text[80];
  canvas.background_opaque();

  _stprintf(text,TEXT("Weight %.0f kg"),
	    GlidePolar::GetAUW());
  canvas.text_opaque(rc.left + IBLSCALE(30), rc.bottom - IBLSCALE(55), text);

  _stprintf(text,TEXT("Wing loading %.1f kg/m2"),
	    GlidePolar::WingLoading);
  canvas.text_opaque(rc.left + IBLSCALE(30), rc.bottom - IBLSCALE(40), text);

  canvas.background_transparent();
}


void FlightStatistics::RenderTask(Canvas &canvas, const RECT rc, const bool olcmode)
{
  int i;
  Chart chart(canvas, rc);
  ScopeLock protect(mutexTaskData);

  double lat1 = 0;
  double lon1 = 0;
  double lat2 = 0;
  double lon2 = 0;
  double x1, y1, x2=0, y2=0;
  double lat_c, lon_c;
  double aatradius[MAXTASKPOINTS];

  // find center

  for (i=0; i<MAXTASKPOINTS; i++) {
    aatradius[i]=0;
  }
  bool nowaypoints = true;

  for (i=0; i<MAXTASKPOINTS; i++) {
    if (ValidTaskPoint(i)) {
      lat1 = WayPointList[Task[i].Index].Latitude;
      lon1 = WayPointList[Task[i].Index].Longitude;
      chart.ScaleYFromValue( lat1);
      chart.ScaleXFromValue(lon1);
      nowaypoints = false;
    }
  }
  if (nowaypoints && !olcmode) {
    chart.DrawNoData();
    return;
  }

  GlideComputer::olc.SetLine();
  int nolc = GlideComputer::olc.getN();
  bool olcvalid = GlideComputer::olc.getValid();
  bool olcfinished = GlideComputer::olc.getFinished();

  if (olcvalid) {
    for (i=0; i< nolc; i++) {
      lat1 = GlideComputer::olc.getLatitude(i);
      lon1 = GlideComputer::olc.getLongitude(i);
      chart.ScaleYFromValue( lat1);
      chart.ScaleXFromValue(lon1);
    }
    if (!olcfinished) {
      lat1 = GlideComputer::olc.lat_proj;
      lon1 = GlideComputer::olc.lon_proj;
      chart.ScaleYFromValue( lat1);
      chart.ScaleXFromValue(lon1);
    }
  }

  lat_c = (chart.getYmax()+chart.getYmin())/2;
  lon_c = (chart.getXmax()+chart.getXmin())/2;

  int nwps = 0;

  // find scale
  chart.ResetScale();

  lat1 = GPS_INFO.Latitude;
  lon1 = GPS_INFO.Longitude;
  x1 = (lon1-lon_c)*fastcosine(lat1);
  y1 = (lat1-lat_c);
  chart.ScaleXFromValue(x1);
  chart.ScaleYFromValue(y1);

  for (i=0; i<MAXTASKPOINTS; i++) {
    if (ValidTaskPoint(i)) {
      nwps++;
      lat1 = WayPointList[Task[i].Index].Latitude;
      lon1 = WayPointList[Task[i].Index].Longitude;
      x1 = (lon1-lon_c)*fastcosine(lat1);
      y1 = (lat1-lat_c);
      chart.ScaleXFromValue(x1);
      chart.ScaleYFromValue(y1);

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
            chart.ScaleXFromValue(x1);
            chart.ScaleYFromValue(y1);
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
  for (i=0; i< nolc; i++) {
    lat1 = GlideComputer::olc.getLatitude(i);
    lon1 = GlideComputer::olc.getLongitude(i);
    x1 = (lon1-lon_c)*fastcosine(lat1);
    y1 = (lat1-lat_c);
    chart.ScaleXFromValue(x1);
    chart.ScaleYFromValue( y1);
  }

  chart.ScaleMakeSquare();
  chart.DrawXGrid(1.0, 0, Chart::STYLE_THINDASHPAPER, 1.0, false);
  chart.DrawYGrid(1.0, 0, Chart::STYLE_THINDASHPAPER, 1.0, false);

  // draw aat areas
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

          canvas.select(MapGfx.GetAirspaceBrushByClass(AATASK));
          canvas.white_pen();
	  if (Task[i].AATType == SECTOR) {
	    canvas.segment(chart.screenX(x2), chart.screenY(y2),
                           chart.screenS(aatradius[i]),
                           rc,
                           Task[i].AATStartRadial,
                           Task[i].AATFinishRadial);
	  } else {
	    canvas.circle(chart.screenX(x2), chart.screenY(y2),
			  chart.screenS(aatradius[i]),
                          rc);
	  }
	}
      }
    }
  }

  // draw track

  for (i=0; i< nolc-1; i++) {
    lat1 = GlideComputer::olc.getLatitude(i);
    lon1 = GlideComputer::olc.getLongitude(i);
    lat2 = GlideComputer::olc.getLatitude(i+1);
    lon2 = GlideComputer::olc.getLongitude(i+1);
    x1 = (lon1-lon_c)*fastcosine(lat1);
    y1 = (lat1-lat_c);
    x2 = (lon2-lon_c)*fastcosine(lat2);
    y2 = (lat2-lat_c);
    chart.DrawLine(x1, y1, x2, y2,
		   Chart::STYLE_MEDIUMBLACK);
  }

  // draw task lines and labels

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

	chart.DrawLine(x1, y1, x2, y2,
		       Chart::STYLE_DASHGREEN);

	TCHAR text[100];
	if ((i==nwps-1) && (Task[i].Index == Task[0].Index)) {
	  _stprintf(text,TEXT("%0d"),1);
	  chart.DrawLabel(text, x2, y2);
	} else {
	  _stprintf(text,TEXT("%0d"),i+1);
	  chart.DrawLabel(text, x2, y2);
	}

	if ((i==ActiveWayPoint)&&(!AATEnabled)) {
	  lat1 = GPS_INFO.Latitude;
	  lon1 = GPS_INFO.Longitude;
	  x1 = (lon1-lon_c)*fastcosine(lat1);
	  y1 = (lat1-lat_c);
	  chart.DrawLine(x1, y1, x2, y2,
			 Chart::STYLE_REDTHICK);
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

	  chart.DrawLine(x1, y1, x2, y2,
			 Chart::STYLE_REDTHICK);
	}
      }
    }
  }

  if (olcmode && olcvalid) {
    for (i=0; i< 7-1; i++) {
      switch(OLCRules) {
      case 0:
	lat1 = GlideComputer::olc.data.solution_FAI_sprint.latitude[i];
	lon1 = GlideComputer::olc.data.solution_FAI_sprint.longitude[i];
	lat2 = GlideComputer::olc.data.solution_FAI_sprint.latitude[i+1];
	lon2 = GlideComputer::olc.data.solution_FAI_sprint.longitude[i+1];
	break;
      case 1:
	lat1 = GlideComputer::olc.data.solution_FAI_triangle.latitude[i];
	lon1 = GlideComputer::olc.data.solution_FAI_triangle.longitude[i];
	lat2 = GlideComputer::olc.data.solution_FAI_triangle.latitude[i+1];
	lon2 = GlideComputer::olc.data.solution_FAI_triangle.longitude[i+1];
	break;
      case 2:
	lat1 = GlideComputer::olc.data.solution_FAI_classic.latitude[i];
	lon1 = GlideComputer::olc.data.solution_FAI_classic.longitude[i];
	lat2 = GlideComputer::olc.data.solution_FAI_classic.latitude[i+1];
	lon2 = GlideComputer::olc.data.solution_FAI_classic.longitude[i+1];
	break;
      }
      x1 = (lon1-lon_c)*fastcosine(lat1);
      y1 = (lat1-lat_c);
      x2 = (lon2-lon_c)*fastcosine(lat2);
      y2 = (lat2-lat_c);
      chart.DrawLine(x1, y1, x2, y2,
		     Chart::STYLE_REDTHICK);
    }
    if (!olcfinished) {
      x1 = (GlideComputer::olc.lon_proj-lon_c)*fastcosine(lat1);
      y1 = (GlideComputer::olc.lat_proj-lat_c);
      chart.DrawLine(x1, y1, x2, y2,
		     Chart::STYLE_BLUETHIN);
    }
  }

  // Draw aircraft on top
  lat1 = GPS_INFO.Latitude;
  lon1 = GPS_INFO.Longitude;
  x1 = (lon1-lon_c)*fastcosine(lat1);
  y1 = (lat1-lat_c);
  chart.DrawLabel(TEXT("+"), x1, y1);
}


void FlightStatistics::RenderTemperature(Canvas &canvas, const RECT rc)
{
  Chart chart(canvas, rc);

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
    chart.DrawNoData();
    return;
  }

  chart.ScaleYFromValue( hmin);
  chart.ScaleYFromValue( hmax);
  chart.ScaleXFromValue(tmin);
  chart.ScaleXFromValue(tmax);

  bool labelDry = false;
  bool labelAir = false;
  bool labelDew = false;

  int ipos = 0;

  for (i=0; i<CUSONDE_NUMLEVELS-1; i++) {

    if (CuSonde::cslevels[i].nmeasurements &&
	CuSonde::cslevels[i+1].nmeasurements) {

      ipos++;

      chart.DrawLine(CuSonde::cslevels[i].tempDry, i,
		     CuSonde::cslevels[i+1].tempDry, (i+1),
		     Chart::STYLE_REDTHICK);

      chart.DrawLine(CuSonde::cslevels[i].airTemp, i,
		     CuSonde::cslevels[i+1].airTemp, (i+1),
		     Chart::STYLE_MEDIUMBLACK);

      chart.DrawLine(CuSonde::cslevels[i].dewpoint, i,
		     CuSonde::cslevels[i+1].dewpoint, i+1,
		     Chart::STYLE_BLUETHIN);

      if (ipos> 2) {
	if (!labelDry) {
	  chart.DrawLabel(TEXT("DALR"), CuSonde::cslevels[i+1].tempDry, i);
	  labelDry = true;
	} else {
	  if (!labelAir) {
	    chart.DrawLabel(TEXT("Air"),
			    CuSonde::cslevels[i+1].airTemp, i);
	    labelAir = true;
	  } else {
	    if (!labelDew) {
	      chart.DrawLabel(TEXT("Dew"),
			      CuSonde::cslevels[i+1].dewpoint, i);
	      labelDew = true;
	    }
	  }
	}
      }
    }
  }

  chart.DrawXLabel(TEXT("T")TEXT(DEG));
  chart.DrawYLabel(TEXT("h"));
}


void FlightStatistics::RenderWind(Canvas &canvas, const RECT rc)
{
  int numsteps=10;
  int i;
  double h;
  Vector wind;
  bool found=true;
  double mag;

  LeastSquares windstats_mag;
  Chart chart(canvas, rc);

  if (Altitude_Ceiling.y_max
      -Altitude_Ceiling.y_min<=10) {
    chart.DrawNoData();
    return;
  }

  for (i=0; i<numsteps ; i++) {

    h = (Altitude_Ceiling.y_max
	 -Altitude_Base.y_min)*
      i/(double)(numsteps-1)+Altitude_Base.y_min;

    wind = GlideComputer::windanalyser->windstore.getWind(GPS_INFO.Time, h, &found);
    mag = sqrt(wind.x*wind.x+wind.y*wind.y);

    windstats_mag.least_squares_update(mag, h);

  }

  //

  chart.ScaleXFromData(&windstats_mag);
  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(10.0);

  chart.ScaleYFromData( &windstats_mag);

  chart.DrawXGrid(5/SPEEDMODIFY, 0, Chart::STYLE_THINDASHPAPER, 5.0, true);
  chart.DrawYGrid(1000/ALTITUDEMODIFY, 0, Chart::STYLE_THINDASHPAPER,
		  1000.0, true);
  chart.DrawLineGraph(&windstats_mag, Chart::STYLE_MEDIUMBLACK);

#define WINDVECTORMAG 25

  numsteps = (int)((rc.bottom-rc.top)/WINDVECTORMAG)-1;

  // draw direction vectors

  double angle;
  double hfact;
  for (i=0; i<numsteps ; i++) {
    hfact = (i+1)/(double)(numsteps+1);
    h = (Altitude_Ceiling.y_max
	 -Altitude_Base.y_min)*
      hfact+Altitude_Base.y_min;

    wind = GlideComputer::windanalyser->windstore.getWind(GPS_INFO.Time, h, &found);
    if (windstats_mag.x_max == 0)
      windstats_mag.x_max=1;  // prevent /0 problems
    wind.x /= windstats_mag.x_max;
    wind.y /= windstats_mag.x_max;
    mag = sqrt(wind.x*wind.x+wind.y*wind.y);
    if (mag<= 0.0) continue;

    angle = atan2(wind.x,-wind.y)*RAD_TO_DEG;

    chart.DrawArrow((chart.getXmin()+chart.getXmax())/2, h,
		    mag*WINDVECTORMAG, angle,
		    Chart::STYLE_MEDIUMBLACK);
  }

  chart.DrawXLabel(TEXT("w"));
  chart.DrawYLabel(TEXT("h"));
}


void FlightStatistics::RenderAirspace(Canvas &canvas, const RECT rc) {
  double range = 50.0*1000; // km
  double aclat, aclon, ach, acb;
  double fi, fj;
  aclat = GPS_INFO.Latitude;
  aclon = GPS_INFO.Longitude;
  ach = GPS_INFO.Altitude;
  acb = GPS_INFO.TrackBearing;
  double hmin = max(0,GPS_INFO.Altitude-3300);
  double hmax = max(3300,GPS_INFO.Altitude+1000);

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

  Chart chart(canvas, rc);
  chart.ResetScale();
  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(range);
  chart.ScaleYFromValue(hmin);
  chart.ScaleYFromValue(hmax);

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

  Pen mpen(Pen::BLANK, 0, RGB(0xf0,0xf0,0xb0));
  canvas.select(mpen);

  RECT rcd;
  for (i=0; i< AIRSPACE_SCANSIZE_H; i++) { // scan height
    rcd.top = chart.screenY(d_h[i]-dfi/2);
    rcd.bottom = chart.screenY(d_h[i]+dfi/2);

    for (j=0; j< AIRSPACE_SCANSIZE_X; j++) { // scan range
      type = d_airspace[i][j];
      if (type>=0) {
        canvas.select(MapGfx.GetAirspaceBrushByClass(type));
        canvas.set_text_color(MapGfx.GetAirspaceColourByClass(type));

	rcd.left = chart.screenX((j-0.5)*dfj*range);
	rcd.right = chart.screenX((j+0.5)*dfj*range);

	canvas.rectangle(rcd.left,rcd.top,rcd.right,rcd.bottom);

      }
    }
  }

  // draw ground
  POINT ground[4];
  Pen penGround(Pen::SOLID, IBLSCALE(1), Chart::GROUND_COLOUR);
  Brush brushGround(Chart::GROUND_COLOUR);
  canvas.select(penGround);
  canvas.select(brushGround);

  for (j=1; j< AIRSPACE_SCANSIZE_X; j++) { // scan range

    ground[0].x = chart.screenX((j-1)*dfj*range);
    ground[1].x = ground[0].x;
    ground[2].x = chart.screenX((j)*dfj*range);
    ground[3].x = ground[2].x;
    ground[0].y = chart.screenY(0);
    ground[1].y = chart.screenY(d_alt[j-1]);
    ground[2].y = chart.screenY(d_alt[j]);
    ground[3].y = ground[0].y;
    canvas.polygon(ground, 4);
  }

  POINT line[4];
  if (GPS_INFO.Speed>10.0) {
    double t = range/GPS_INFO.Speed;
    double gfh = (ach+CALCULATED_INFO.Average30s*t-hmin)/(hmax-hmin);
    line[0].x = rc.left;
    line[0].y = chart.screenY(fh);
    line[1].x = rc.right;
    line[1].y = chart.screenY(gfh);
    chart.StyleLine(line[0], line[1], Chart::STYLE_BLUETHIN);
  }

  canvas.white_pen();
  canvas.white_brush();
  SetTextColor(canvas, RGB(0xff,0xff,0xff));

  chart.DrawXGrid(5.0/DISTANCEMODIFY, 0,
		  Chart::STYLE_THINDASHPAPER, 5.0, true);
  chart.DrawYGrid(1000.0/ALTITUDEMODIFY, 0, Chart::STYLE_THINDASHPAPER,
		  1000.0, true);

  // draw aircraft
  int delta;
  canvas.white_pen();
  canvas.white_brush();

  line[0].x = chart.screenX(0.0);
  line[0].y = chart.screenY(ach);
  line[1].x = rc.left;
  line[1].y = line[0].y;
  delta = (line[0].x-line[1].x);
  line[2].x = line[1].x;
  line[2].y = line[0].y-delta/2;
  line[3].x = (line[1].x+line[0].x)/2;
  line[3].y = line[0].y;
  Polygon(canvas, line, 4);

  chart.DrawXLabel(TEXT("D"));
  chart.DrawYLabel(TEXT("h"));
}
