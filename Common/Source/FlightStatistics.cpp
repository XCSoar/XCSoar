/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Math/Constants.h"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "Units.hpp"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "OnLineContest.h"
#include "WindStore.h"
#include "Language.hpp"
#include "MacCready.h"
#include "Atmosphere.h"
#include "SettingsComputer.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

#include "GlideSolvers/GlidePolar.hpp"

#ifndef _MSC_VER
#include <algorithm>
using std::min;
using std::max;
#endif

void FlightStatistics::Reset() {
  Lock();
  ThermalAverage.Reset();
  Wind_x.Reset();
  Wind_y.Reset();
  Altitude.Reset();
  Altitude_Base.Reset();
  Altitude_Ceiling.Reset();
  Task_Speed.Reset();
  Altitude_Terrain.Reset();
#ifdef OLD_TASK
  for(int j=0;j<MAXTASKPOINTS;j++) {
    LegStartTime[j] = -1;
  }
#endif
  Unlock();
}

#include "Screen/Chart.hpp"

void
FlightStatistics::RenderBarograph(Canvas &canvas, const RECT rc,
                                  const DERIVED_INFO &derived) const
{
  Chart chart(canvas, rc);

  if (Altitude.sum_n<2) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleXFromData(Altitude);
  chart.ScaleYFromData(Altitude);
  chart.ScaleYFromValue(0);
  chart.ScaleXFromValue(Altitude.x_min+1.0); // in case no data
  chart.ScaleXFromValue(Altitude.x_min);

#ifdef OLD_TASK
  for(int j=1; task.ValidTaskPoint(j) ;j++) {
    if (LegStartTime[j]>=0) {
      double xx = (LegStartTime[j] - derived.TakeOffTime) / 3600.0;
      if (xx>=0) {
        chart.DrawLine(xx, chart.getYmin(),
		       xx, chart.getYmax(),
		       Chart::STYLE_REDTHICK);
      }
    }
  }

  Pen hpHorizonGround(Pen::SOLID, IBLSCALE(1), Chart::GROUND_COLOUR);
  Brush hbHorizonGround(Chart::GROUND_COLOUR);

  canvas.select(hpHorizonGround);
  canvas.select(hbHorizonGround);

  chart.DrawFilledLineGraph(Altitude_Terrain, Chart::GROUND_COLOUR);
  canvas.white_pen();
  canvas.white_brush();

  chart.DrawXGrid(0.5, Altitude.x_min, Chart::STYLE_THINDASHPAPER, 0.5, true);
  chart.DrawYGrid(1000/ALTITUDEMODIFY, 0, Chart::STYLE_THINDASHPAPER, 1000, true);
  chart.DrawLineGraph(Altitude, Chart::STYLE_MEDIUMBLACK);

  chart.DrawTrend(Altitude_Base, Chart::STYLE_BLUETHIN);
  chart.DrawTrend(Altitude_Ceiling, Chart::STYLE_BLUETHIN);

  chart.DrawXLabel(TEXT("t"));
  chart.DrawYLabel(TEXT("h"));
#endif
}

void
FlightStatistics::RenderSpeed(Canvas &canvas, const RECT rc,
                              const DERIVED_INFO &derived) const
{
  Chart chart(canvas, rc);

#ifdef OLD_TASK
  if ((Task_Speed.sum_n<2)
      || !task.Valid()) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleXFromData(Task_Speed);
  chart.ScaleYFromData(Task_Speed);
  chart.ScaleYFromValue( 0);
  chart.ScaleXFromValue(Task_Speed.x_min+1.0); // in case no data
  chart.ScaleXFromValue(Task_Speed.x_min);

  for(int j=1;task.ValidTaskPoint(j);j++) {
    if (LegStartTime[j]>=0) {
      double xx = (LegStartTime[j] - derived.TaskStartTime) / 3600.0;
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
  chart.DrawLineGraph(Task_Speed, Chart::STYLE_MEDIUMBLACK);
  chart.DrawTrend(Task_Speed, Chart::STYLE_BLUETHIN);

  chart.DrawXLabel(TEXT("t"));
  chart.DrawYLabel(TEXT("V"));
#endif
}

void
FlightStatistics::RenderClimb(Canvas &canvas, const RECT rc,
                              const GlidePolar& glide_polar) const
{
  Chart chart(canvas, rc);

  if (ThermalAverage.sum_n<1) {
    chart.DrawNoData();
    return;
  }
  double MACCREADY = glide_polar.get_mc();

  chart.ScaleYFromData(ThermalAverage);
  chart.ScaleYFromValue( (MACCREADY+0.5));
  chart.ScaleYFromValue( 0);

  chart.ScaleXFromValue(-1);
  chart.ScaleXFromValue(ThermalAverage.sum_n);

  chart.DrawYGrid(1.0/LIFTMODIFY, 0, Chart::STYLE_THINDASHPAPER, 1.0, true);
  chart.DrawBarChart(ThermalAverage);

  chart.DrawLine(0, MACCREADY, ThermalAverage.sum_n,
		 MACCREADY, Chart::STYLE_REDTHICK);

  chart.DrawLabel(TEXT("MC"), max(0.5, (double)ThermalAverage.sum_n - 1.0),
                  MACCREADY);

  chart.DrawTrendN(ThermalAverage, Chart::STYLE_BLUETHIN);

  chart.DrawXLabel(TEXT("n"));
  chart.DrawYLabel(TEXT("w"));
}

void
FlightStatistics::RenderGlidePolar(Canvas &canvas, const RECT rc,
                                   const DERIVED_INFO &derived,
                                   const SETTINGS_COMPUTER &settings_computer,
                                   const GlidePolar& glide_polar) const
{
  int i;
  Chart chart(canvas, rc);

  chart.ScaleYFromValue( 0);
  chart.ScaleYFromValue(-glide_polar.get_Smax()*1.1);
  chart.ScaleXFromValue(glide_polar.get_Vmin()*0.8);
  chart.ScaleXFromValue(glide_polar.get_Vmax() + 2);

  chart.DrawXGrid(10.0/SPEEDMODIFY, 0,
		  Chart::STYLE_THINDASHPAPER, 10.0, true);
  chart.DrawYGrid(1.0/LIFTMODIFY, 0,
		  Chart::STYLE_THINDASHPAPER, 1.0, true);

  double sinkrate0, sinkrate1;
  double v0=0, v1;
  bool v0valid = false;
  int i0=0;

  for (i= glide_polar.get_Vmin(); i <= glide_polar.get_Vmax();
       ++i) {

    sinkrate0 = -glide_polar.SinkRate(i);
    sinkrate1 = -glide_polar.SinkRate(i+1);
    chart.DrawLine(i, sinkrate0 ,
		   i+1, sinkrate1,
		   Chart::STYLE_MEDIUMBLACK);

    if (derived.AverageClimbRateN[i] > 0) {
      v1 = derived.AverageClimbRate[i] / derived.AverageClimbRateN[i];

      if (v0valid) {
        chart.DrawLine(i0, v0 ,
		       i, v1,
		       Chart::STYLE_DASHGREEN);
      }

      v0 = v1; i0 = i;
      v0valid = true;
    }
  }

  double MACCREADY = glide_polar.get_mc();

  double ff = glide_polar.get_Vmax()
    / max(1.0, derived.VMacCready);
  double sb = -glide_polar.SinkRate(derived.VMacCready);
  ff = (sb - MACCREADY) / max(1.0, derived.VMacCready);

  chart.DrawLine(0, MACCREADY, glide_polar.get_Vmax(),
                 MACCREADY + ff * glide_polar.get_Vmax(),
		 Chart::STYLE_REDTHICK);

  chart.DrawXLabel(TEXT("V"));
  chart.DrawYLabel(TEXT("w"));

  TCHAR text[80];
  canvas.background_opaque();

  _stprintf(text,TEXT("Weight %d kg"),
	    glide_polar.get_all_up_weight().as_int());
  canvas.text_opaque(rc.left + IBLSCALE(30), rc.bottom - IBLSCALE(55), text);

  _stprintf(text,TEXT("Wing loading %.1f kg/m2"),
	    glide_polar.get_wing_loading().as_double());
  canvas.text_opaque(rc.left + IBLSCALE(30), rc.bottom - IBLSCALE(40), text);

  canvas.background_transparent();
}

void
FlightStatistics::RenderTask(Canvas &canvas, const RECT rc,
                             const NMEA_INFO &nmea_info,
                             const SETTINGS_COMPUTER &settings_computer,
                             const SETTINGS_MAP &settings_map,
                             const OLCOptimizer &olc, const bool olcmode) const
{
#ifdef OLD_TASK
  int i;
  Chart chart(canvas, rc);

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

  for (i=0; task.ValidTaskPoint(i); i++) {
    lat1 = task.getTaskPointLocation(i).Latitude;
    lon1 = task.getTaskPointLocation(i).Longitude;
    chart.ScaleYFromValue( lat1);
    chart.ScaleXFromValue(lon1);
    nowaypoints = false;
  }

  if (nowaypoints && !olcmode) {
    chart.DrawNoData();
    return;
  }

  int nolc = olc.getN();
  bool olcvalid = olc.getValid(settings_computer);
  bool olcfinished = olc.getFinished(settings_computer);

  if (olcvalid) {
    for (i=0; i< nolc; i++) {
      lat1 = olc.getLocation(i).Latitude;
      lon1 = olc.getLocation(i).Longitude;
      chart.ScaleYFromValue( lat1);
      chart.ScaleXFromValue(lon1);
    }
    if (!olcfinished) {
      lat1 = olc.loc_proj.Latitude;
      lon1 = olc.loc_proj.Longitude;
      chart.ScaleYFromValue( lat1);
      chart.ScaleXFromValue(lon1);
    }
  }

  lat_c = (chart.getYmax()+chart.getYmin())/2;
  lon_c = (chart.getXmax()+chart.getXmin())/2;

  int nwps = 0;

  // find scale
  chart.ResetScale();

  lat1 = nmea_info.Location.Latitude;
  lon1 = nmea_info.Location.Longitude;
  x1 = (lon1-lon_c)*fastcosine(lat1);
  y1 = (lat1-lat_c);
  chart.ScaleXFromValue(x1);
  chart.ScaleYFromValue(y1);

  for (i=0; task.ValidTaskPoint(i); i++) {
    nwps++;
    lat1 = task.getTaskPointLocation(i).Latitude;
    lon1 = task.getTaskPointLocation(i).Longitude;
    x1 = (lon1-lon_c)*fastcosine(lat1);
    y1 = (lat1-lat_c);
    chart.ScaleXFromValue(x1);
    chart.ScaleYFromValue(y1);

    if (task.getSettings().AATEnabled) {
      GEOPOINT aatloc;
      double bearing;
      double radius;

      if (task.ValidTaskPoint(i+1)) {
        if (task.getTaskPoint(i).AATType == AAT_SECTOR) {
          radius = task.getTaskPoint(i).AATSectorRadius;
        } else {
          radius = task.getTaskPoint(i).AATCircleRadius;
        }
        for (int j=0; j<4; j++) {
          bearing = j*360.0/4;

          FindLatitudeLongitude(task.getTaskPointLocation(i),
                                bearing, radius,
                                &aatloc);
          x1 = (aatloc.Longitude-lon_c)*fastcosine(aatloc.Latitude);
          y1 = (aatloc.Latitude-lat_c);
          chart.ScaleXFromValue(x1);
          chart.ScaleYFromValue(y1);
          if (j==0) {
            aatradius[i] = fabs(aatloc.Latitude -
                                task.getTaskPointLocation(i).Latitude);
          }
        }
      } else {
        aatradius[i] = 0;
      }
    }
  }

  for (i=0; i< nolc; i++) {
    lat1 = olc.getLocation(i).Latitude;
    lon1 = olc.getLocation(i).Longitude;
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
    if (task.getSettings().AATEnabled) {
      for (i=MAXTASKPOINTS-1; i>0; i--) {
	if (task.ValidTaskPoint(i)) {
          lat1 = task.getTaskPointLocation(i-1).Latitude;
          lon1 = task.getTaskPointLocation(i-1).Longitude;
          lat2 = task.getTaskPointLocation(i).Latitude;
          lon2 = task.getTaskPointLocation(i).Longitude;
	  x1 = (lon1-lon_c)*fastcosine(lat1);
	  y1 = (lat1-lat_c);
	  x2 = (lon2-lon_c)*fastcosine(lat2);
	  y2 = (lat2-lat_c);

          canvas.select(MapGfx.GetAirspaceBrushByClass(AATASK, settings_map));
          canvas.white_pen();
	  if (task.getTaskPoint(i).AATType == AAT_SECTOR) {
	    canvas.segment(chart.screenX(x2), chart.screenY(y2),
                           chart.screenS(aatradius[i]),
                           rc,
                           task.getTaskPoint(i).AATStartRadial,
                           task.getTaskPoint(i).AATFinishRadial);
	  } else {
            canvas.autoclip_circle(chart.screenX(x2), chart.screenY(y2),
                                   chart.screenS(aatradius[i]), rc);
	  }
	}
      }
    }
  }

  // draw track

  for (i=0; i< nolc-1; i++) {
    lat1 = olc.getLocation(i).Latitude;
    lon1 = olc.getLocation(i).Longitude;
    lat2 = olc.getLocation(i+1).Latitude;
    lon2 = olc.getLocation(i+1).Longitude;
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
      if (task.ValidTaskPoint(i) && task.ValidTaskPoint(i-1)) {
        lat1 = task.getTaskPointLocation(i-1).Latitude;
        lon1 = task.getTaskPointLocation(i-1).Longitude;
	if (task.TaskIsTemporary()) {
	  lat2 = nmea_info.Location.Latitude;
	  lon2 = nmea_info.Location.Longitude;
	} else {
          lat2 = task.getTaskPointLocation(i).Latitude;
          lon2 = task.getTaskPointLocation(i).Longitude;
	}
	x1 = (lon1-lon_c)*fastcosine(lat1);
	y1 = (lat1-lat_c);
	x2 = (lon2-lon_c)*fastcosine(lat2);
	y2 = (lat2-lat_c);

	chart.DrawLine(x1, y1, x2, y2,
		       Chart::STYLE_DASHGREEN);

	TCHAR text[100];
	if ((i==nwps-1) &&
            (task.getWaypointIndex(i) == task.getWaypointIndex(0))) {
	  _stprintf(text,TEXT("%0d"),1);
	  chart.DrawLabel(text, x2, y2);
	} else {
	  _stprintf(text,TEXT("%0d"),i+1);
	  chart.DrawLabel(text, x2, y2);
	}

	if ((i==(int)task.getActiveIndex())&&(!task.getSettings().AATEnabled)) {
	  lat1 = nmea_info.Location.Latitude;
	  lon1 = nmea_info.Location.Longitude;
	  x1 = (lon1-lon_c)*fastcosine(lat1);
	  y1 = (lat1-lat_c);
	  chart.DrawLine(x1, y1, x2, y2,
			 Chart::STYLE_REDTHICK);
	}

      }
    }

    // draw aat task line

    if (task.getSettings().AATEnabled) {
      for (i=0; task.ValidTaskPoint(i) && task.ValidTaskPoint(i+1); i++) {
        GEOPOINT loc1 = task.getTargetLocation(i);
        GEOPOINT loc2 = task.getTargetLocation(i+1);

        x1 = (loc1.Longitude-lon_c)*fastcosine(loc1.Latitude);
        y1 = (loc1.Latitude-lat_c);
        x2 = (loc2.Longitude-lon_c)*fastcosine(loc2.Latitude);
        y2 = (loc2.Latitude-lat_c);

        chart.DrawLine(x1, y1, x2, y2,
                       Chart::STYLE_REDTHICK);
      }
    }
  }

  if (olcmode && olcvalid) {
    for (i=0; i< 7-1; i++) {
      switch (settings_computer.OLCRules) {
      case 0:
	lat1 = olc.data.solution_FAI_sprint.location[i].Latitude;
	lon1 = olc.data.solution_FAI_sprint.location[i].Longitude;
	lat2 = olc.data.solution_FAI_sprint.location[i+1].Latitude;
	lon2 = olc.data.solution_FAI_sprint.location[i+1].Longitude;
	break;
      case 1:
	lat1 = olc.data.solution_FAI_triangle.location[i].Latitude;
	lon1 = olc.data.solution_FAI_triangle.location[i].Longitude;
	lat2 = olc.data.solution_FAI_triangle.location[i+1].Latitude;
	lon2 = olc.data.solution_FAI_triangle.location[i+1].Longitude;
	break;
      case 2:
	lat1 = olc.data.solution_FAI_classic.location[i].Latitude;
	lon1 = olc.data.solution_FAI_classic.location[i].Longitude;
	lat2 = olc.data.solution_FAI_classic.location[i+1].Latitude;
	lon2 = olc.data.solution_FAI_classic.location[i+1].Longitude;
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
      x1 = (olc.loc_proj.Longitude-lon_c)*fastcosine(lat1);
      y1 = (olc.loc_proj.Latitude-lat_c);
      chart.DrawLine(x1, y1, x2, y2,
		     Chart::STYLE_BLUETHIN);
    }
  }

  // Draw aircraft on top
  lat1 = nmea_info.Location.Latitude;
  lon1 = nmea_info.Location.Longitude;
  x1 = (lon1-lon_c)*fastcosine(lat1);
  y1 = (lat1-lat_c);
  chart.DrawLabel(TEXT("+"), x1, y1);
#endif
}

void
FlightStatistics::RenderTemperature(Canvas &canvas, const RECT rc) const
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

      hmin = min(hmin, (float)i);
      hmax = max(hmax, (float)i);
      tmin = min(tmin, (float)min(CuSonde::cslevels[i].tempDry,
                                  min(CuSonde::cslevels[i].airTemp,
			       CuSonde::cslevels[i].dewpoint)));
      tmax = max(tmax, (float)max(CuSonde::cslevels[i].tempDry,
                                  (double)max(CuSonde::cslevels[i].airTemp,
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

void
FlightStatistics::RenderWind(Canvas &canvas, const RECT rc,
                             const NMEA_INFO &nmea_info,
                             const WindStore &wind_store) const
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

    wind = wind_store.GetWind(nmea_info.Time, h, &found);
    mag = sqrt(wind.x*wind.x+wind.y*wind.y);

    windstats_mag.LeastSquaresUpdate(mag, h);

  }

  //

  chart.ScaleXFromData(windstats_mag);
  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(10.0);

  chart.ScaleYFromData(windstats_mag);

  chart.DrawXGrid(5/SPEEDMODIFY, 0, Chart::STYLE_THINDASHPAPER, 5.0, true);
  chart.DrawYGrid(1000/ALTITUDEMODIFY, 0, Chart::STYLE_THINDASHPAPER,
		  1000.0, true);
  chart.DrawLineGraph(windstats_mag, Chart::STYLE_MEDIUMBLACK);

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

    wind = wind_store.GetWind(nmea_info.Time, h, &found);
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


#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"

class AirspaceIntersectionVisitorSlice: 
  public AirspaceIntersectionVisitor {
public:
  AirspaceIntersectionVisitorSlice(Canvas &canvas,
                                   Chart &chart,
                                   const SETTINGS_MAP &settings,
                                   const GEOPOINT start):
    m_canvas(canvas),
    m_chart(chart),
    m_settings(settings),
    m_start(start)
    {      
      Pen mpen(Pen::BLANK, 0, Color(0xf0,0xf0,0xb0));
      m_canvas.select(mpen);
    };

  void Render(const AbstractAirspace& as, int type) {
    if (m_intersections.empty()) {
      return;
    }

    m_canvas.select(MapGfx.GetAirspaceBrushByClass(type, m_settings));
    m_canvas.set_text_color(MapGfx.GetAirspaceColourByClass(type,
                                                          m_settings));

    RECT rcd;
    rcd.top = m_chart.screenY(as.get_top_altitude());
    if (as.is_base_terrain()) {
      rcd.bottom = m_chart.screenY(0);
    } else {
      rcd.bottom = m_chart.screenY(as.get_base_altitude());
    }
    
    for (AirspaceIntersectionVector::const_iterator it = m_intersections.begin();
         it != m_intersections.end(); ++it) {
      const GEOPOINT p_start = (it->first);
      const GEOPOINT p_end = (it->second);
      const fixed distance_start = m_start.distance(p_start);
      const fixed distance_end = m_start.distance(p_end);

      rcd.left = m_chart.screenX(distance_start);
      rcd.right = m_chart.screenX(distance_end);
      m_canvas.rectangle(rcd.left,rcd.top,rcd.right,rcd.bottom);
    }
  }

  void render(const AbstractAirspace& as) {
    int type = as.get_type();
    if (type>=0) {
      Render(as, type);
    }
  }
  void Visit(const AirspaceCircle& as) {
    render(as);
  }
  void Visit(const AirspacePolygon& as) {
    render(as);
  }
private:
  Canvas& m_canvas;
  Chart& m_chart;
  const SETTINGS_MAP& m_settings;
  const GEOPOINT& m_start;
};



void
FlightStatistics::RenderAirspace(Canvas &canvas, const RECT rc,
                                 const NMEA_INFO &nmea_info,
                                 const DERIVED_INFO &derived,
                                 const SETTINGS_MAP &settings_map,
                                 const Airspaces &airspace_database,
                                 RasterTerrain &terrain) const
{
  fixed range = 50000; // 50 km
  fixed hmin = max(fixed_zero, nmea_info.GPSAltitude - fixed(3300));
  fixed hmax = max(fixed(3300), nmea_info.GPSAltitude + fixed(1000));
  const GEOPOINT p_start = nmea_info.Location;
  const GeoVector vec(range, nmea_info.TrackBearing);
  const GEOPOINT p_end = vec.end_point(p_start);

  Chart chart(canvas, rc);
  chart.ResetScale();
  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(range);
  chart.ScaleYFromValue(hmin);
  chart.ScaleYFromValue(hmax);

  // draw airspaces
  AirspaceIntersectionVisitorSlice ivisitor(canvas, chart, settings_map, p_start);
  airspace_database.visit_intersecting(p_start, vec, ivisitor, true);

  // draw terrain
  terrain.Lock();
  if (terrain.GetMap()) {
    // want most accurate rounding here
    RasterRounding rounding(*terrain.GetMap(),0,0);

    std::vector<POINT> points;
    POINT pf0, pf1;
    pf0.x = chart.screenX(range);
    pf0.y = chart.screenY(0);
    points.push_back(pf0);
    pf1.x = chart.screenX(0);
    pf1.y = chart.screenY(0);
    points.push_back(pf1);

    for (unsigned j=0; j< AIRSPACE_SCANSIZE_X; ++j) {

      const fixed t_this = fixed(j)/(AIRSPACE_SCANSIZE_X-1);
      const GEOPOINT p_this = p_start+(p_end-p_start)*t_this;

      POINT p; 
      p.x = chart.screenX(t_this*range);
      p.y = chart.screenY(terrain.GetTerrainHeight(p_this, rounding));
      
      points.push_back(p);
    }
    points.push_back(points[0]);

    canvas.select(Pen(Pen::SOLID, IBLSCALE(1), Chart::GROUND_COLOUR));
    canvas.select(Brush(Chart::GROUND_COLOUR));
    canvas.polygon(&points[0], points.size());
  }
  terrain.Unlock();

  // draw aircraft trend line
  if (nmea_info.Speed>10.0) {
    fixed t = range/nmea_info.Speed;
    chart.DrawLine(0, nmea_info.GPSAltitude, 
                   range, nmea_info.GPSAltitude+derived.Average30s*t,
                   Chart::STYLE_BLUETHIN);
  }

  // draw aircraft
  {
    int delta;
    canvas.white_pen();
    canvas.white_brush();
    
    POINT line[4];
    line[0].x = chart.screenX(0.0);
    line[0].y = chart.screenY(nmea_info.GPSAltitude);
    line[1].x = rc.left;
    line[1].y = line[0].y;
    delta = (line[0].x-line[1].x);
    line[2].x = line[1].x;
    line[2].y = line[0].y-delta/2;
    line[3].x = (line[1].x+line[0].x)/2;
    line[3].y = line[0].y;
    canvas.polygon(line, 4);
  }

  // draw grid
  {
    canvas.white_pen();
    canvas.white_brush();
    canvas.set_text_color(Color(0xff,0xff,0xff));
    
    chart.DrawXGrid(5.0/DISTANCEMODIFY, 0,
                    Chart::STYLE_THINDASHPAPER, 5.0, true);
    chart.DrawYGrid(1000.0/ALTITUDEMODIFY, 0, Chart::STYLE_THINDASHPAPER,
                    1000.0, true);
    
    chart.DrawXLabel(TEXT("D"));
    chart.DrawYLabel(TEXT("h"));
  }
}

void
FlightStatistics::StartTask(double starttime)
{
#ifdef OLD_TASK
  Lock();
  LegStartTime[0] = starttime;
  LegStartTime[1] = starttime;
  // JMW clear thermal climb average on task start
  ThermalAverage.Reset();
  Task_Speed.Reset();
  Unlock();
#endif
}


void
FlightStatistics::AddAltitudeTerrain(const double tflight,
				     const double terrainalt)
{
  Lock();
  Altitude_Terrain.LeastSquaresUpdate(max(0.0, tflight / 3600.0),
                                        terrainalt);
  Unlock();
}

void
FlightStatistics::AddAltitude(const double tflight,
			      const double alt)
{
  Lock();
  Altitude.LeastSquaresUpdate(max(0.0, tflight / 3600.0), alt);
  Unlock();
}

double
FlightStatistics::AverageThermalAdjusted
(const double mc_current,
 const bool circling)
{
  double mc_stats;
  Lock();
  if (ThermalAverage.y_ave>0) {
    if ((mc_current>0) && circling) {
      mc_stats = (ThermalAverage.sum_n*ThermalAverage.y_ave
		  +mc_current)/(ThermalAverage.sum_n+1);
    } else {
      mc_stats = ThermalAverage.y_ave;
    }
  } else {
    mc_stats = mc_current;
  }
  Unlock();
  return mc_stats;
}

void
FlightStatistics::SaveTaskSpeed(const double val)
{
  Task_Speed.LeastSquaresUpdate(val);
}


void
FlightStatistics::SetLegStart(const int activewaypoint,
			      const double time)
{
#ifdef OLD_TASK
  Lock();
  if (LegStartTime[task.getActiveIndex()]<0) {
    LegStartTime[task.getActiveIndex()] = time;
  }
  Unlock();
#endif
}

void
FlightStatistics::AddClimbBase(const double tflight,
			       const double alt)
{
  Lock();
  if (Altitude_Ceiling.sum_n>0) {
    // only update base if have already climbed, otherwise
    // we will catch the takeoff height as the base.

    Altitude_Base.LeastSquaresUpdate(max(0.0, tflight) / 3600.0,
				       alt);
  }
  Unlock();
}


void
FlightStatistics::AddClimbCeiling(const double tflight,
			       const double alt)
{
  Lock();
  Altitude_Ceiling.LeastSquaresUpdate(max(0.0, tflight) / 3600.0,
					alt);
  Unlock();
}

/**
 * Adds a thermal to the ThermalAverage calculator
 * @param v Average climb speed of the last thermal
 */
void
FlightStatistics::AddThermalAverage(const double v)
{
  Lock();
  ThermalAverage.LeastSquaresUpdate(v);
  Unlock();
}

void
FlightStatistics::CaptionBarograph(TCHAR *sTmp)
{
  Lock();
  if (Altitude_Ceiling.sum_n<2) {
    _stprintf(sTmp, TEXT("\0"));
  } else if (Altitude_Ceiling.sum_n<4) {
    _stprintf(sTmp, TEXT("%s:\r\n  %.0f-%.0f %s"),
	      gettext(TEXT("Working band")),
	      Altitude_Base.y_ave*ALTITUDEMODIFY,
	      Altitude_Ceiling.y_ave*ALTITUDEMODIFY,
	      Units::GetAltitudeName());
  } else {
    _stprintf(sTmp, TEXT("%s:\r\n  %.0f-%.0f %s\r\n\r\n%s:\r\n  %.0f %s/hr"),
	      gettext(TEXT("Working band")),
	      Altitude_Base.y_ave*ALTITUDEMODIFY,
              Altitude_Ceiling.y_ave*ALTITUDEMODIFY,
	      Units::GetAltitudeName(),
	      gettext(TEXT("Ceiling trend")),
	      Altitude_Ceiling.m*ALTITUDEMODIFY,
	      Units::GetAltitudeName());
  }
  Unlock();
}

void
FlightStatistics::CaptionClimb( TCHAR* sTmp)
{
  Lock();
  if (ThermalAverage.sum_n==0) {
    _stprintf(sTmp, TEXT("\0"));
  } else if (ThermalAverage.sum_n==1) {
    _stprintf(sTmp, TEXT("%s:\r\n  %3.1f %s"),
	      gettext(TEXT("Av climb")),
	      ThermalAverage.y_ave*LIFTMODIFY,
	      Units::GetVerticalSpeedName()
	      );
  } else {
    _stprintf(sTmp, TEXT("%s:\r\n  %3.1f %s\r\n\r\n%s:\r\n  %3.2f %s"),
	      gettext(TEXT("Av climb")),
	      ThermalAverage.y_ave*LIFTMODIFY,
	      Units::GetVerticalSpeedName(),
	      gettext(TEXT("Climb trend")),
	      ThermalAverage.m*LIFTMODIFY,
	      Units::GetVerticalSpeedName()
	      );
  }
  Unlock();
}


void
FlightStatistics::CaptionPolar(TCHAR *sTmp) const
{
  if (Layout::landscape) {
    _stprintf(sTmp, TEXT("%s:\r\n  %3.0f\r\n  at %3.0f %s\r\n\r\n%s:\r\n%3.2f %s\r\n  at %3.0f %s"),
	      gettext(TEXT("Best LD")),
	      oldGlidePolar::bestld,
	      oldGlidePolar::Vbestld*SPEEDMODIFY,
	      Units::GetHorizontalSpeedName(),
	      gettext(TEXT("Min sink")),
	      oldGlidePolar::minsink*LIFTMODIFY,
	      Units::GetVerticalSpeedName(),
	      oldGlidePolar::Vminsink*SPEEDMODIFY,
	      Units::GetHorizontalSpeedName()
	      );
  } else {
    _stprintf(sTmp, TEXT("%s:\r\n  %3.0f at %3.0f %s\r\n%s:\r\n  %3.2f %s at %3.0f %s"),
	      gettext(TEXT("Best LD")),
	      oldGlidePolar::bestld,
	      oldGlidePolar::Vbestld*SPEEDMODIFY,
	      Units::GetHorizontalSpeedName(),
	      gettext(TEXT("Min sink")),
	      oldGlidePolar::minsink*LIFTMODIFY,
	      Units::GetVerticalSpeedName(),
	      oldGlidePolar::Vminsink*SPEEDMODIFY,
	      Units::GetHorizontalSpeedName());
  }
}


void
FlightStatistics::CaptionTempTrace(TCHAR *sTmp) const
{
  _stprintf(sTmp, TEXT("%s:\r\n  %5.0f %s\r\n\r\n%s:\r\n  %5.0f %s\r\n"),
	    gettext(TEXT("Thermal height")),
	    CuSonde::thermalHeight*ALTITUDEMODIFY,
	    Units::GetAltitudeName(),
	    gettext(TEXT("Cloud base")),
	    CuSonde::cloudBase*ALTITUDEMODIFY,
	    Units::GetAltitudeName());
}

void
FlightStatistics::CaptionTask(TCHAR *sTmp,
                              const DERIVED_INFO &derived) const
{
#ifdef OLD_TASK
  if (!task.Valid()) {
    _stprintf(sTmp, gettext(TEXT("No task")));
  } else {
    TCHAR timetext1[100];
    TCHAR timetext2[100];
    if (task.getSettings().AATEnabled) {
      Units::TimeToText(timetext1, (int)derived.TaskTimeToGo);
      Units::TimeToText(timetext2, (int)derived.AATTimeToGo);

      if (Layout::landscape) {
	_stprintf(sTmp,
		  TEXT("%s:\r\n  %s\r\n%s:\r\n  %s\r\n%s:\r\n  %5.0f %s\r\n%s:\r\n  %5.0f %s\r\n"),
		  gettext(TEXT("Task to go")),
		  timetext1,
                    gettext(TEXT("AAT to go")),
		  timetext2,
		  gettext(TEXT("Distance to go")),
                  DISTANCEMODIFY * derived.AATTargetDistance,
		  Units::GetDistanceName(),
		  gettext(TEXT("Target speed")),
                  TASKSPEEDMODIFY * derived.AATTargetSpeed,
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
                  DISTANCEMODIFY * derived.AATTargetDistance,
		  Units::GetDistanceName(),
		  gettext(TEXT("Target speed")),
                  TASKSPEEDMODIFY * derived.AATTargetSpeed,
		  Units::GetTaskSpeedName()
		  );
      }
    } else {
      Units::TimeToText(timetext1, (int)derived.TaskTimeToGo);
      _stprintf(sTmp, TEXT("%s: %s\r\n%s: %5.0f %s\r\n"),
		gettext(TEXT("Task to go")),
		timetext1,
		gettext(TEXT("Distance to go")),
                DISTANCEMODIFY * derived.TaskDistanceToGo,
		Units::GetDistanceName());
    }
  }
#else
    _stprintf(sTmp, gettext(TEXT("No task")));
#endif
}
