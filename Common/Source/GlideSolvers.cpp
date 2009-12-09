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

/**
 * Final glide through terrain and footprint calculations
 * @file GlideSolvers.cpp
 */

#include "GlideSolvers.hpp"
#include "McCready.h"
#include "Task.h"
#include "Math/Earth.hpp"
#include "Components.hpp"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"

double
PirkerAnalysis(const NMEA_INFO &basic, const DERIVED_INFO &calculated,
               const double this_bearing,
               const double GlideSlope)
{
  // bool maxfound = false;
  // bool first = true;
  double pirker_mc = 0.0;
  double h_target = GlideSlope;
  double h;
  double dh = 1.0;
  double last_pirker_mc = 5.0;
  double last_dh = -1.0;
  double pirker_mc_zero = 0.0;

  (void)basic;

  while (pirker_mc < 10.0) {

    h = GlidePolar::MacCreadyAltitude(pirker_mc, 1.0, this_bearing,
        calculated.WindSpeed, calculated.WindBearing, 0, 0, true, 0);

    dh = (h_target - h);
    // height difference, how much we have compared to
    // how much we need at that speed.
    //   dh>0, we can afford to speed up

    if (dh == last_dh) {
      // same height, must have hit max speed.
      if (dh > 0)
        return last_pirker_mc;
      else
        return 0.0;
    }

    if ((dh <= 0) && (last_dh > 0)) {
      if (dh - last_dh < 0) {
        double f = (-last_dh) / (dh - last_dh);
        pirker_mc_zero = last_pirker_mc * (1.0 - f) + f * pirker_mc;
      } else {
        pirker_mc_zero = pirker_mc;
      }

      return pirker_mc_zero;
    }
    last_dh = dh;
    last_pirker_mc = pirker_mc;

    pirker_mc += 0.5;
  }

  if (dh >= 0) {
    return pirker_mc;
  }

  return -1.0; // no solution found, unreachable without further climb
}

double
MacCreadyTimeLimit(const NMEA_INFO &basic, const DERIVED_INFO &calculated,
                   const double this_bearing,
                   const double time_remaining,
                   const double h_final)
{
  // find highest Mc to achieve greatest distance in remaining time and height
  (void)basic;

  double time_to_go;
  double mc;
  double mc_best = 0.0;
  double d_best = 0.0;
  const double windspeed = calculated.WindSpeed;
  const double windbearing = calculated.WindBearing;
  const double navaltitude = calculated.NavAltitude;

  for (mc = 0; mc < 10.0; mc += 0.1) {
    double h_unit = GlidePolar::MacCreadyAltitude(mc, 1.0, this_bearing,
        windspeed, windbearing, NULL, NULL, 1, &time_to_go);

    if (time_to_go > 0) {
      double p = time_remaining / time_to_go;
      double h_spent = h_unit * p;
      double dh = navaltitude - h_spent - h_final;
      double d = 1.0 * p;

      if ((d > d_best) && (dh >= 0))
        mc_best = mc;
    }
  }

  return mc_best;
}

static double
EffectiveMacCready_internal(const NMEA_INFO &basic,
                            const DERIVED_INFO &calculated,
                            bool cruise_efficiency_mode)
{
  if (calculated.ValidFinish)
    return 0;
  if (task.getActiveIndex() <= 0)
    // no e mc before start
    return 0;
  if (!calculated.ValidStart)
    return 0;
  if (calculated.TaskStartTime < 0)
    return 0;
  if (!task.Valid() || !task.ValidTaskPoint(task.getActiveIndex() - 1))
    return 0;
  if (calculated.TaskDistanceToGo <= 0)
    return 0;

  double mc_setting = GlidePolar::GetMacCready();

  double start_speed = calculated.TaskStartSpeed;
  double V_bestld = GlidePolar::Vbestld;
  double energy_height_start =
      max(0, start_speed * start_speed - V_bestld * V_bestld) / (9.81 * 2.0);

  double telapsed = basic.Time - calculated.TaskStartTime;
  double height_below_start =
    calculated.TaskStartAltitude + energy_height_start
    - calculated.NavAltitude - calculated.EnergyHeight;

  double LegDistances[MAXTASKPOINTS];
  double LegBearings[MAXTASKPOINTS];

  // JMW TODO remove dist/bearing: this is already done inside the task!
  for (unsigned i = 0; i < task.getActiveIndex(); i++) {
    GEOPOINT w1 = task.getTargetLocation(i + 1);
    GEOPOINT w0 = task.getTargetLocation(i);

    DistanceBearing(w0, w1, &LegDistances[i], &LegBearings[i]);

    if (i + 1 == task.getActiveIndex()) {
      LegDistances[i] = ProjectedDistance(w0, w1, basic.Location);
    }
    if ((task.getSettings().StartType == START_CIRCLE) && (i == 0)) {
      // Correct speed calculations for radius
      // JMW TODO accuracy: leg distance replace this with more accurate version
      // leg_distance -= StartRadius;
      LegDistances[0] = max(0.1, LegDistances[0] - task.getSettings().StartRadius);
    }
  }

  // OK, distance/bearings calculated, now search for Mc
  double value_found;
  if (cruise_efficiency_mode)
    value_found = 1.5; // max
  else
    value_found = 10.0; // max

  for (double value_scan = 0.01; value_scan < 1.0; value_scan += 0.01) {
    double height_remaining = height_below_start;
    double time_total=0;

    double mc_effective;
    double cruise_efficiency;

    if (cruise_efficiency_mode) {
      mc_effective = mc_setting;
      if (calculated.FinalGlide && calculated.timeCircling > 0) {
        mc_effective = calculated.TotalHeightClimb / calculated.timeCircling;
      }
      cruise_efficiency = 0.5 + value_scan;
    } else {
      mc_effective = value_scan * 10.0;
      cruise_efficiency = 1.0;
    }

    // Now add times from start to this waypoint,
    // allowing for final glide where possible if aircraft height is below
    // start
    for(int i=task.getActiveIndex()-1;i>=0; i--) {
      double time_this;

      double height_used_this =
        GlidePolar::MacCreadyAltitude(mc_effective,
                                      LegDistances[i],
                                      LegBearings[i],
                                      calculated.WindSpeed,
                                      calculated.WindBearing,
                                      0, NULL,
                                      (height_remaining>0),
                                      &time_this,
                                      height_remaining,
                                      cruise_efficiency);

      height_remaining -= height_used_this;

      if (time_this >= 0) {
        time_total += time_this;
      } else {
        // invalid! break out of loop early
        time_total = time_this;
        i = -1;
        continue;
      }
    }

    if (time_total < 0) {
      // invalid
      continue;
    }
    if (time_total > telapsed) {
      // already too slow
      continue;
    }

    // add time for climb from start height to height above start
    if (height_below_start < 0) {
      time_total -= height_below_start / mc_effective;
    }
    // now check time..
    if (time_total < telapsed) {
      if (cruise_efficiency_mode) {
        value_found = cruise_efficiency;
      } else {
        value_found = mc_effective;
      }
      break;
    }
  }

  return value_found;
}

double
EffectiveCruiseEfficiency(const NMEA_INFO &basic,
                          const DERIVED_INFO &calculated)
{
  double value = EffectiveMacCready_internal(basic, calculated, true);
  if (value < 0.75) {
    return 0.75;
  }
  return value;
}

double
EffectiveMacCready(const NMEA_INFO &basic, const DERIVED_INFO &calculated)
{
  return EffectiveMacCready_internal(basic, calculated, false);
}
