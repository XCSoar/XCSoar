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

#include "GlideSolvers.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "McReady.h"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "RasterTerrain.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "WayPoint.hpp"
#include "Components.hpp"

//////////////////////////////////////////////////////////
// Final glide through terrain and footprint calculations

double
FinalGlideThroughTerrain(const double this_bearing,
                         const NMEA_INFO *Basic,
                         const DERIVED_INFO *Calculated,
			 const SETTINGS_COMPUTER &settings,
                         double *retlat, double *retlon,
                         const double max_range,
                         bool *out_of_range,
                         double *TerrainBase)
{
  double mc = GlidePolar::GetMacCready();
  double irange = GlidePolar::MacCreadyAltitude(mc,
						1.0, this_bearing,
						Calculated->WindSpeed,
						Calculated->WindBearing,
						0, 0, true, 0);
  const double start_lat = Basic->Latitude;
  const double start_lon = Basic->Longitude;
  if (retlat && retlon) {
    *retlat = start_lat;
    *retlon = start_lon;
  }
  *out_of_range = false;

  if ((irange<=0.0)||(Calculated->NavAltitude<=0)) {
    // can't make progress in this direction at the current windspeed/mc
    return 0;
  }

  const double glide_max_range = Calculated->NavAltitude/irange;

  // returns distance one would arrive at altitude in straight glide
  // first estimate max range at this altitude
  double lat, lon;
  double last_lat, last_lon;
  double h=0.0, dh=0.0;
//  int imax=0;
  double last_dh=0;
  double altitude;

  terrain.Lock();
  double retval = 0;
  int i=0;
  bool start_under = false;

  // calculate terrain rounding factor

  FindLatitudeLongitude(start_lat, start_lon, 0,
                        glide_max_range/NUMFINALGLIDETERRAIN, &lat, &lon);

  double Xrounding = fabs(lon-start_lon)/2;
  double Yrounding = fabs(lat-start_lat)/2;
  terrain.SetTerrainRounding(Xrounding, Yrounding);

  lat = last_lat = start_lat;
  lon = last_lon = start_lon;

  altitude = Calculated->NavAltitude;
  h =  max(0, terrain.GetTerrainHeight(lat, lon));
  dh = altitude - h - settings.SAFETYALTITUDETERRAIN;
  last_dh = dh;
  if (dh<0) {
    start_under = true;
    // already below safety terrain height
    //    retval = 0;
    //    goto OnExit;
  }

  // find grid
  double dlat, dlon;

  FindLatitudeLongitude(lat, lon, this_bearing, glide_max_range, &dlat, &dlon);
  dlat -= start_lat;
  dlon -= start_lon;

  double f_scale = 1.0/NUMFINALGLIDETERRAIN;
  if ((max_range>0) && (max_range<glide_max_range)) {
    f_scale *= max_range/glide_max_range;
  }

  double delta_alt = -f_scale*Calculated->NavAltitude;

  dlat *= f_scale;
  dlon *= f_scale;

  for (i=1; i<=NUMFINALGLIDETERRAIN; i++) {
    double f;
    bool solution_found = false;
    double fi = i*f_scale;
    // fraction of glide_max_range

    if ((max_range>0)&&(fi>=1.0)) {
      // early exit
      *out_of_range = true;
      retval = max_range;
      goto OnExit;
    }

    if (start_under) {
      altitude += 2.0*delta_alt;
    } else {
      altitude += delta_alt;
    }

    // find lat, lon of point of interest

    lat += dlat;
    lon += dlon;

    // find height over terrain
    h =  max(0,terrain.GetTerrainHeight(lat, lon));

    dh = altitude - h - settings.SAFETYALTITUDETERRAIN;

    if (TerrainBase && (dh>0) && (h>0)) {
      *TerrainBase = min(*TerrainBase, h);
    }

    if (start_under) {
      if (dh>last_dh) {
	// better solution found, ok to continue...
	if (dh>0) {
	  // we've now found a terrain point above safety altitude,
	  // so consider rest of track to search for safety altitude
	  start_under = false;
	}
      } else {
	f= 0.0;
	solution_found = true;
      }
    } else if (dh<=0) {
      if ((dh<last_dh) && (last_dh>0)) {
        f = max(0,min(1,(-last_dh)/(dh-last_dh)));
      } else {
	f = 0.0;
      }
      solution_found = true;
    }
    if (solution_found) {
      double distance;
      lat = last_lat*(1.0-f)+lat*f;
      lon = last_lon*(1.0-f)+lon*f;
      if (retlat && retlon) {
        *retlat = lat;
        *retlon = lon;
      }
      DistanceBearing(start_lat, start_lon, lat, lon, &distance, NULL);
      retval = distance;
      goto OnExit;
    }
    last_dh = dh;
    last_lat = lat;
    last_lon = lon;
  }

  *out_of_range = true;
  retval = glide_max_range;

 OnExit:
  terrain.Unlock();
  return retval;
}

double
PirkerAnalysis(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated,
               const double this_bearing,
               const double GlideSlope)
{
//  bool maxfound = false;
//  bool first = true;
  double pirker_mc = 0.0;
  double h_target = GlideSlope;
  double h;
  double dh= 1.0;
  double last_pirker_mc = 5.0;
  double last_dh = -1.0;
  double pirker_mc_zero = 0.0;

  (void)Basic;

  while (pirker_mc<10.0) {

    h = GlidePolar::MacCreadyAltitude(pirker_mc,
                                      1.0, // unit distance
				      this_bearing,
                                      Calculated->WindSpeed,
                                      Calculated->WindBearing,
                                      0, 0, true, 0);

    dh = (h_target-h);
    // height difference, how much we have compared to
    // how much we need at that speed.
    //   dh>0, we can afford to speed up

    if (dh==last_dh) {
      // same height, must have hit max speed.
      if (dh>0) {
        return last_pirker_mc;
      } else {
        return 0.0;
      }
    }

    if ((dh<=0)&&(last_dh>0)) {
      if (dh-last_dh < 0) {
	double f = (-last_dh)/(dh-last_dh);
	pirker_mc_zero = last_pirker_mc*(1.0-f)+f*pirker_mc;
      } else {
	pirker_mc_zero = pirker_mc;
      }
      return pirker_mc_zero;
    }
    last_dh = dh;
    last_pirker_mc = pirker_mc;

    pirker_mc += 0.5;
  }
  if (dh>=0) {
    return pirker_mc;
  }
  return -1.0; // no solution found, unreachable without further climb
}

double
MacCreadyTimeLimit(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated,
                   const double this_bearing,
                   const double time_remaining,
                   const double h_final)
{
  // find highest Mc to achieve greatest distance in remaining time and height
  (void)Basic;

  double time_to_go;
  double mc;
  double mc_best = 0.0;
  double d_best = 0.0;
  const double windspeed =   Calculated->WindSpeed;
  const double windbearing = Calculated->WindBearing;
  const double navaltitude = Calculated->NavAltitude;

  for (mc=0; mc<10.0; mc+= 0.1) {

    double h_unit = GlidePolar::MacCreadyAltitude(mc,
						 1.0, // unit distance
						 this_bearing,
						 windspeed,
						 windbearing,
						 NULL,
						 NULL,
						 1, // final glide
						 &time_to_go);
    if (time_to_go>0) {
      double p = time_remaining/time_to_go;
      double h_spent = h_unit*p;
      double dh = navaltitude-h_spent-h_final;
      double d = 1.0*p;

      if ((d>d_best) && (dh>=0)) {
	mc_best = mc;
      }
    }
  }
  return mc_best;
}

static double
EffectiveMacCready_internal(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated,
                            bool cruise_efficiency_mode)
{
  if (Calculated->ValidFinish) return 0;
  if (ActiveTaskPoint<=0) return 0; // no e mc before start
  if (!Calculated->ValidStart) return 0;
  if (Calculated->TaskStartTime<0) return 0;


  if (!ValidTask()
      || !ValidTaskPoint(ActiveTaskPoint-1)) return 0;
  if (Calculated->TaskDistanceToGo<=0) {
    return 0;
  }

  double mc_setting = GlidePolar::GetMacCready();

  mutexTaskData.Lock();

  double start_speed = Calculated->TaskStartSpeed;
  double V_bestld = GlidePolar::Vbestld;
  double energy_height_start =
    max(0, start_speed*start_speed-V_bestld*V_bestld)/(9.81*2.0);

  double telapsed = Basic->Time-Calculated->TaskStartTime;
  double height_below_start =
    Calculated->TaskStartAltitude + energy_height_start
    - Calculated->NavAltitude - Calculated->EnergyHeight;

  double LegDistances[MAXTASKPOINTS];
  double LegBearings[MAXTASKPOINTS];

  for (int i=0; i<ActiveTaskPoint; i++) {
    double w1lat = WayPointList[task_points[i+1].Index].Latitude;
    double w1lon = WayPointList[task_points[i+1].Index].Longitude;
    double w0lat = WayPointList[task_points[i].Index].Latitude;
    double w0lon = WayPointList[task_points[i].Index].Longitude;
    if (AATEnabled) {
      if (ValidTaskPoint(i+1)) {
        w1lat = task_stats[i+1].AATTargetLat;
        w1lon = task_stats[i+1].AATTargetLon;
      }
      if (i>0) {
        w0lat = task_stats[i].AATTargetLat;
        w0lon = task_stats[i].AATTargetLon;
      }
    }
    DistanceBearing(w0lat,
                    w0lon,
                    w1lat,
                    w1lon,
                    &LegDistances[i], &LegBearings[i]);

    if (i==ActiveTaskPoint-1) {

      double leg_covered = ProjectedDistance(w0lon, w0lat,
                                             w1lon, w1lat,
                                             Basic->Longitude,
                                             Basic->Latitude);
      LegDistances[i] = leg_covered;
    }
    if ((StartLine==0) && (i==0)) {
      // Correct speed calculations for radius
      // JMW TODO accuracy: leg distance replace this with more accurate version
      // leg_distance -= StartRadius;
      LegDistances[0] = max(0.1,LegDistances[0]-StartRadius);
    }
  }

  // OK, distance/bearings calculated, now search for Mc

  double value_found;
  if (cruise_efficiency_mode) {
    value_found = 1.5; // max
  } else {
    value_found = 10.0; // max
  }

  for (double value_scan=0.01; value_scan<1.0; value_scan+= 0.01) {

    double height_remaining = height_below_start;
    double time_total=0;

    double mc_effective;
    double cruise_efficiency;

    if (cruise_efficiency_mode) {
      mc_effective = mc_setting;
      if (Calculated->FinalGlide && (Calculated->timeCircling>0)) {
	mc_effective = Calculated->TotalHeightClimb
	  /Calculated->timeCircling;
      }
      cruise_efficiency = 0.5+value_scan;
    } else {
      mc_effective = value_scan*10.0;
      cruise_efficiency = 1.0;
    }

    // Now add times from start to this waypoint,
    // allowing for final glide where possible if aircraft height is below
    // start

    for(int i=ActiveTaskPoint-1;i>=0; i--) {

      double time_this;

      double height_used_this =
        GlidePolar::MacCreadyAltitude(mc_effective,
                                      LegDistances[i],
                                      LegBearings[i],
                                      Calculated->WindSpeed,
                                      Calculated->WindBearing,
                                      0, NULL,
                                      (height_remaining>0),
                                      &time_this,
                                      height_remaining,
				      cruise_efficiency);

      height_remaining -= height_used_this;

      if (time_this>=0) {
        time_total += time_this;
      } else {
        // invalid! break out of loop early
        time_total= time_this;
        i= -1;
        continue;
      }
    }

    if (time_total<0) {
      // invalid
      continue;
    }
    if (time_total>telapsed) {
      // already too slow
      continue;
    }

    // add time for climb from start height to height above start
    if (height_below_start<0) {
      time_total -= height_below_start/mc_effective;
    }
    // now check time..
    if (time_total<telapsed) {
      if (cruise_efficiency_mode) {
	value_found = cruise_efficiency;
      } else {
	value_found = mc_effective;
      }
      break;
    }

  }

  mutexTaskData.Unlock();

  return value_found;
}

double
EffectiveCruiseEfficiency(const NMEA_INFO *Basic,
                          const DERIVED_INFO *Calculated)
{
  double value = EffectiveMacCready_internal(Basic, Calculated, true);
  if (value<0.75) {
    return 0.75;
  }
  return value;
}

double
EffectiveMacCready(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated)
{
  return EffectiveMacCready_internal(Basic, Calculated, false);
}
