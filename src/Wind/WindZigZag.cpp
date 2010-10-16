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

#include "Wind/WindZigZag.hpp"
#include "LogFile.hpp"
#include "Math/FastMath.h"
#include "Math/Angle.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"

#include <math.h>

#ifndef _MSC_VER
  #include <algorithm>
  using std::min;
  using std::max;
#endif

/**
 * number of points required, should be
 * equivalent to a circle to prevent
 * zigzag from dominating when used in
 * conjunction with circling drift
 * calculation.
 */
#define NUM_SAMPLES 20

/**
 * equivalent to a circle to prevent
 * zigzag from dominating when used in
 * conjunction with circling drift
 * calculation.
 */
static const fixed SAMPLE_RATE(4);

/**
 * minimum number of seconds between
 * recalculating estimate
 */
static const fixed UPDATE_RATE(20);

/** 15 m/s = 30 knots max approx */
static const int V_SCALE = 20;
/** resolution on V search */
#define NUM_V_POINTS 41
/** resolution on theta search */
#define NUM_THETA_POINTS (36)

static int
VtoI(fixed V)
{
  return min(NUM_V_POINTS - 1, iround(V * (NUM_V_POINTS - 1) / V_SCALE));
}

static fixed
ItoV(int i)
{
  return fixed(max(0, min(i, NUM_V_POINTS - 1)) * V_SCALE / (NUM_V_POINTS - 1));
}

/**
 * Class for points used in ZigZag wind algorithm
 */
class ZigZagPoint
{
public:
  ZigZagPoint() :
    time(-1)
  {
    for (int i = 0; i < NUM_V_POINTS; i++)
      theta_west_ok[i] = false;
  }

  fixed V_tas;
  fixed V_gps;
  Angle theta_gps;
  fixed time;

  void
  Set(fixed t, fixed aV_tas, fixed aV_gps, Angle atheta_gps)
  {
    V_tas = aV_tas;
    V_gps = aV_gps;
    time = t;
    theta_gps = atheta_gps;
    CalculateThetaWEst();
  }

  Angle theta_west_1[NUM_V_POINTS];
  Angle theta_west_2[NUM_V_POINTS];
  bool theta_west_ok[NUM_V_POINTS];

  fixed cos_theta_gps;
  fixed sin_theta_gps;

private:
  int V_gps_x;
  int V_gps_y;
  int V_tas_l;

  void
  CalculateThetaWEst()
  {
    int i;
    for (i = 0; i < NUM_V_POINTS; i++) {
      fixed Vwest = ItoV(i);
      theta_west_ok[i] = EstimateW0(Vwest, i);
    }

    cos_theta_gps = theta_gps.cos();
    sin_theta_gps = theta_gps.sin();

    V_gps_x = iround(100 * V_gps * cos_theta_gps);
    V_gps_y = iround(100 * V_gps * sin_theta_gps);
    V_tas_l = iround(100 * V_tas);
  }

  bool
  EstimateW0(fixed Vwest, int i)
  {
    fixed west_rat = Vwest / V_tas;
    fixed gps_rat = V_gps / V_tas;

    if (gps_rat < fixed(0.001))
      // speed too small
      return false;

    if (gps_rat + west_rat < fixed_one)
      // wind too weak
      return false;

    if ((Vwest > V_gps) && (Vwest > V_tas))
      // wind too strong
      return false;

    if (west_rat < fixed(0.001)) {
      // wind speed too small
      theta_west_1[i] = Angle::native(fixed_zero);
      theta_west_2[i] = Angle::native(fixed_zero);
      return true;
    }

    fixed cosgamma = (west_rat * west_rat + gps_rat * gps_rat - fixed_one)
                      / (2 * west_rat * gps_rat);

    if (fabs(cosgamma) > fixed_one)
      return false;

    Angle gamma = Angle::radians(acos(cosgamma));
    theta_west_1[i] = (Angle::radians(fixed_pi) - theta_gps - gamma).as_delta().flipped();
    theta_west_2[i] = (Angle::radians(fixed_pi) - theta_gps + gamma).as_delta().flipped();

    return true;
  }

public:
  int
  EstimateW1(int V_west_x, int V_west_y)
  {
    int v_tas_x = V_gps_x + V_west_x;
    int v_tas_y = V_gps_y + V_west_y;
    long vv = isqrt4(v_tas_x * v_tas_x + v_tas_y * v_tas_y);
    long vdiff = (long)V_tas_l - vv;
    int err = (1000 * max(vdiff, -vdiff)) / V_tas_l;
    // returns error in tenths of percent
    return err;
  }
};

class ZigZag
{
public:
  ZigZagPoint points[NUM_SAMPLES];

  ZigZag()
  {
    for (int i = 0; i < NUM_SAMPLES; i++)
      points[i].time = fixed_minus_one;

    for (int k = 0; k < NUM_THETA_POINTS; k++)
      thetalist[k] = Angle::radians(k * fixed_two_pi / NUM_THETA_POINTS).as_delta();
  }

  void
  AddPoint(fixed t, fixed V_tas, fixed V_gps, Angle theta_gps)
  {
    // find oldest point
    fixed toldest = fixed_zero;
    int ioldest = 0;
    int i;

    for (i = 0; i < NUM_SAMPLES; i++)
      if (t < points[i].time)
        points[i].time = fixed_minus_one;

    for (i = 0; i < NUM_SAMPLES; i++) {
      if ((points[i].time < toldest) || (i == 0) || (t < points[i].time)) {
        toldest = points[i].time;
        ioldest = i;
      }
    }

    i = ioldest;
    points[i].Set(t, V_tas, V_gps, theta_gps);
  }

  fixed
  CheckValidity(fixed t)
  {
    // requires:
    // -- all points to be initialised
    // -- all points to be within last 10 minutes
    int nf = 0;
    fixed ctg = fixed_zero;
    fixed stg = fixed_zero;
    int i;

    for (i = 0; i < NUM_SAMPLES; i++) {
      if (positive(points[i].time)) {
        nf++;
        if (t - points[i].time > fixed(10 * 60)) {
          // clear point so it gets filled next time
          points[i].time = fixed_minus_one;
          return fixed_minus_one;
        }
        ctg += points[i].cos_theta_gps;
        stg += points[i].sin_theta_gps;
      }
    }

    if (nf < NUM_SAMPLES)
      return fixed_minus_one;

    Angle theta_av = Angle::radians(atan2(stg, ctg));
    fixed dtheta_max = fixed_zero;
    fixed dtheta_min = fixed_zero;

    for (i = 0; i < NUM_SAMPLES; i++) {
      fixed da = (points[i].theta_gps - theta_av).as_delta().value_radians();

      if (da > dtheta_max)
        dtheta_max = da;

      if (da < dtheta_min)
        dtheta_min = da;
    }

    return dtheta_max - dtheta_min;
  }

  bool
  CheckSpread(fixed time, fixed error)
  {
    fixed spread = CheckValidity(time);
    if (negative(spread))
      return false;

    spread /= fixed_deg_to_rad;
    fixed minspread;

    minspread = fixed(10 + 30) / (fixed_one + error * error / 30);
    // nominal spread required is 40 degrees, smaller if large
    // error is detected

    if ((spread > fixed_360) || (spread < minspread))
      // invalid if really circling or if not enough zig-zag
      return false;

    return true;
  }

private:
  Angle thetalist[NUM_THETA_POINTS];

  // search all points for error against theta at wind speed index j
  fixed
  AngleError(Angle theta, int j)
  {
    fixed de = fixed_zero;
    int nf = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
      if (points[i].theta_west_ok[j]) {
        fixed e1 = (theta - points[i].theta_west_1[j]).
            as_delta().magnitude_radians();
        fixed e2 = (theta - points[i].theta_west_2[j]).
            as_delta().magnitude_radians();
        if (e1 <= e2)
          de += e1 * e1;
        else
          de += e2 * e2;

        nf++;
      }
    }

    if (nf > 0)
      return de / nf;
    else
      return fixed_minus_one;
  }

  // search for theta to give best match at wind speed index i
  bool
  FindBestAngle(int i, Angle &theta_best)
  {
    fixed debest(1000000);
    bool ok = false;

    for (int k = 0; k < NUM_THETA_POINTS; k++) {
      fixed ae = AngleError(thetalist[k], i);
      if (negative(ae))
        continue;

      ok = true;
      if (ae < debest) {
        debest = ae;
        theta_best = thetalist[k];
      }
    }

    return ok;
  }

  // find average error in true airspeed given wind estimate
  int
  VError(int V_west_x, int V_west_y)
  {
    int verr = 0;
    int nf = 0;

    for (int j = 0; j < NUM_SAMPLES; j++) {
      if (points[j].time >= fixed_zero) {
        verr += points[j].EstimateW1(V_west_x, V_west_y);
        nf++;
      }
    }

    if (nf > 0)
      return verr / nf;
    else
      return 1000;
  }

  bool
  UpdateSearch_Inner(fixed V_west, Angle theta_best)
  {
    // given wind speed and direction, find TAS error
    int V_west_x = iround(100 * V_west * theta_best.cos());
    int V_west_y = iround(100 * V_west * theta_best.sin());
    int verr = VError(V_west_x, V_west_y);

    // search for minimum error
    // (this is not monotonous)
    if (verr >= error_best)
      return false;

    error_best = verr;
    V_west_best = V_west;
    theta_west_best = theta_best;

    return true;
  }

  bool
  UpdateSearch(fixed V_west)
  {
    int i = VtoI(V_west);

    // find best angle estimate for this assumed wind speed i
    Angle theta;
    if (!FindBestAngle(i, theta))
      theta = theta_west_best;

    return (UpdateSearch_Inner(V_west, theta)
            || UpdateSearch_Inner(V_west, theta_west_best));
  }

public:
  int error_best;
  fixed V_west_best;
  Angle theta_west_best;

  fixed
  StartSearch(fixed V_start, Angle theta_start)
  {
    V_west_best = V_start;
    theta_west_best = theta_start;
    error_best = 10000;
    UpdateSearch(V_start);
    V_west_best = V_start;
    theta_west_best = theta_start;
    return fixed(error_best) / 10;
  }

  bool
  Estimate(fixed &V_westb, Angle &theta_westb, fixed &error)
  {
    int i;

    bool scanned[NUM_V_POINTS];
    for (i = 0; i < NUM_V_POINTS; i++)
      scanned[i] = false;

    // scan for 6 points around current best estimate.
    // if a better estimate is found, keep scanning around
    // that point, and don't repeat scans

    bool improved = false;
    bool continue_search = true;
    bool full_search = false;

    while (continue_search) {
      continue_search = false;

      int ib = VtoI(V_west_best);
      int il, ih;
      if (full_search) {
        il = 0;
        ih = NUM_V_POINTS - 1;
      } else {
        il = min(NUM_V_POINTS - 1, max(0, ib - 3));
        ih = min(NUM_V_POINTS - 1, max(0, ib + 3));
      }
      for (i = il; i <= ih; i++) {
        if (scanned[i]) {
          continue;
        } else {
          scanned[i] = true;
          // see if we can find a better estimate
          fixed V_west = ItoV(i);
          if (UpdateSearch(V_west)) {
            improved = true;
            continue_search = true; // earnt more search
          }
        }
      }

      if (!continue_search && !full_search && (error_best > 100)) {
        full_search = true;
        continue_search = true;
        // if no improvement and still large error,
        // try searching all speeds that haven't been checked yet.
      }
    }

    // return true if estimate was improved
    V_westb = V_west_best;
    theta_westb = theta_west_best.as_bearing();

    error = fixed(error_best) / 10;
    return improved;
  }
};

ZigZag myzigzag;

static bool
WindZigZagCheckAirData(const NMEA_INFO &basic)
{
  static fixed tLast(-1);
  static Angle bearingLast = Angle::native(fixed_zero);

  if (!basic.flight.Flying ||
      fabs(basic.TurnRate) > fixed(20) ||
      fabs(basic.GroundSpeed) < fixed(2.5) ||
      fabs(basic.acceleration.Gload - fixed_one) > fixed(0.3)) {
    tLast = basic.Time; // blackout for SAMPLE_RATE seconds
    return false;
  }

  if (basic.Time < tLast + SAMPLE_RATE &&
      (bearingLast - basic.TrackBearing).as_delta().magnitude_degrees() < fixed(10))
    return false;

  tLast = basic.Time;
  bearingLast = basic.TrackBearing;

  return true;
}

int
WindZigZagUpdate(const NMEA_INFO &basic, const DERIVED_INFO &derived,
                 fixed &zzwindspeed, fixed &zzwindbearing)
{
  static fixed tLastEstimate(-1);

  if (!basic.AirspeedAvailable)
    return 0;

  // TODO accuracy: correct TAS for vertical speed if dynamic pullup

  if ((basic.Time <= tLastEstimate) || (tLastEstimate == fixed_minus_one))
    tLastEstimate = basic.Time - UPDATE_RATE;

  if (!WindZigZagCheckAirData(basic))
    return 0;

  // ok to add a point

  myzigzag.AddPoint(basic.Time,
                    basic.TrueAirspeed, basic.GroundSpeed,
                    basic.TrackBearing);

  // don't update wind from zigzag more often than
  // every UPDATE_RATE seconds, so it is balanced with respect
  // to circling
  if (basic.Time < tLastEstimate + UPDATE_RATE)
    return 0;

  fixed V_wind_estimate = basic.wind.norm;
  Angle theta_wind_estimate = basic.wind.bearing;
  fixed percent_error = myzigzag.StartSearch(V_wind_estimate,
                                             theta_wind_estimate);

  // Check spread of zig-zag manoeuver
  if (!myzigzag.CheckSpread(basic.Time, percent_error))
    return 0;

  fixed v_error = percent_error * basic.TrueAirspeed / 100;

  if (v_error < fixed_half)
    // don't refine search if error is small
    return 0;

  if (myzigzag.Estimate(V_wind_estimate, theta_wind_estimate, percent_error)) {
    // ok, we have made an update
    tLastEstimate = basic.Time;

    zzwindspeed = V_wind_estimate;
    zzwindbearing = theta_wind_estimate.value_degrees();

    // calculate error quality
    int quality;

    //double pes = v_error/(V_SCALE/NUM_V_POINTS);
    //quality = iround(0.5+4.5/(1.0+percent_error*percent_error/30.0));

    quality = max(1, 5 - iround(percent_error / 2));
    if (derived.Circling)
      quality = max(1, quality / 2); // de-value updates in circling mode

    return quality;
  }

  return 0;
}

