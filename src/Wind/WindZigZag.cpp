/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include <algorithm>

using std::min;
using std::max;

/**
 * number of points required, should be
 * equivalent to a circle to prevent
 * zigzag from dominating when used in
 * conjunction with circling drift
 * calculation.
 */
#define NUM_SAMPLES 20

/**
 * time to not add points after flight condition is false
 */
#define BLACKOUT_TIME 3

/*
  Unconstrained LSQ problem
    min S(beta)
      S(beta) = sum_k=1..K (y_k-Phi(x_k, beta))^2
              = sum_k=1..K f(x_k,y_k, beta)^2

    f(y_k,x_k, beta) = y_k-Phi(x_k, beta)^2
    y = Phi(x, beta) is model function

    n = dim(beta)
    K = number of data points

    y: true airspeed
    x: vector of gps ground speed north, east
    beta: vector of wind speed north, east
    Phi: sqrt( (x_north+beta_north)^2 + (x_east+beta_east)^2 )
*/

#include <list>
#include "Math/newuoa.hh"

/**
 * This class provides a mechanism to estimate the wind speed and bearing
 * for an aircraft assumed to be flying level such that the airspeed vector
 * is in the horizontal plane, and has access to true airspeed and GPS speed
 * measurements.  No compass information is required by the algorithm.
 *
 * In order to resolve reasonable estimates, and in particular to
 * avoid an ambiguity regarding the hidden state variable of aircraft
 * heading, it is necessary that the aircraft performs bearing
 * adjustments during the sampling.  One way of achieving this is to
 * fly a zig-zag manoeuver; although the aircraft can perform any manoeuver in
 * which the bearing changes.
 *
 * However, due to the condition requiring the airspeed vector to be
 * predominantly in the horizontal plane, it is necessary to filter
 * out observations during high G-loading manoeuvers since for these
 * states the condition cannot be reliably assured.  Consequently,
 * this implementation uses a 'blackout' of a few seconds, in which no
 * samples are accepted, whenever the load magnitude deviates from one
 * by more than 0.3 g.
 *
 * The true airspeed measurements are assumed to be affected by noise
 * of zero mean; if the noise is not zero mean, then the resulting
 * wind estimate will be biased if the samples do not cover a large
 * variation of bearings.
 *
 * The algorithm also checks linearity of the estimated TAS against the
 * measured TAS.  The Pearson correlation coefficient, r (indicating linearity),
 * is used to reject solutions if r is low; secondly, the slope of the
 * TAS to estimated TAS is determined.  This could be used to provide a linear
 * calibration coefficient for the TAS (currently unused).
 *
 * This algorithm was developed by John Wharington.
 */
class WindZigZag {
public:
  struct ZZX {
    ZZX(const fixed mag, const Angle ang) {
      ang.sin_cos(gps_east, gps_north);
      gps_north *= mag;
      gps_east *= mag;
    }
    fixed gps_north;
    fixed gps_east;
  };

  struct ZZY {
    ZZY(const fixed _tas): tas(_tas) {}
    const fixed tas;
  };

  struct ZZBeta {
    ZZBeta(fixed _north, fixed _east):north(_north),east(_east) {}
    const fixed north;
    const fixed east;
  };

  struct ZZObs {
    ZZObs(const unsigned _time, const fixed _speed, const Angle _track,
          const fixed _tas)
      :time(_time), x(_speed, _track), y(_tas), track(_track) {}

    const unsigned time;
    const ZZX x;
    const ZZY y;
    const Angle track;

    fixed Phi(const ZZBeta& beta) const {
      return hypot(x.gps_north + beta.north, x.gps_east + beta.east);
    }

    fixed f(const ZZBeta& beta) const {
      return y.tas - Phi(beta);
    }

    fixed mag() const {
      return y.tas;
    }
  };

private:
  typedef std::list<ZZObs> ObsList;

  ObsList obs;
  unsigned time_blackout;

public:

  WindZigZag() {
    reset();
  }

  /**
   * Clear as if never used
   */
  void reset() {
    obs.clear();
    time_blackout = (unsigned)-1;
  }

  /**
   * Find optimal value
   */
  ZZBeta optimise(int &res_quality, const SpeedVector start,
                  const bool circling);

  /**
   * Operator used by newuoa optimiser to evaluate function to be
   * minimised
   */
  double operator()(int n, double* x) const {
    return fmin(ZZBeta((fixed)x[0], (fixed)x[1]));
  }

protected:

  bool full() const {
    return obs.size()== NUM_SAMPLES;
  }

  bool back_in_time(const unsigned time);

  /**
   * Determine whether the time and angle satisfy criterion for
   * adding it to the list
   */
  bool do_append(const unsigned time, const Angle &ang) const;

  /**
   * Set blackout timer
   */
  void blackout(const unsigned time);

  /**
   * Add observation (dropping out oldest if necessary), and reset
   * blackout timer.
   */
  void append(const ZZObs &o);

private:
  unsigned size() const {
    return obs.size();
  }

  /**
   * Remove items which have high relative error (leaving at least some items).
   * This is to remove outliers which are likely to be caused by manoeuvering.
   * On average, more outliers than useful values are likely to be removed
   * by this algorithm.
   */
  void prune_worst(const ZZBeta &beta);

  std::pair<fixed, fixed> correlation(const ZZBeta& beta) const;

  /**
   * Convenience routine, return squared value
   */
  static fixed sqr(const fixed &v) {
    return v*v;
  }

  /**
   * Function to be optimised (minimised)
   * This is the sum of squared errors between TAS and estimated TAS
   */
  fixed fmin(const ZZBeta& beta) const;

  /**
   * Remove item which has worst error.
   */
  void remove_worst(const ZZBeta& beta);

  /**
   * Find error
   */
  fixed relative_error_squared(const ZZBeta& beta) const;

  /**
   * Calculate quality of fit of wind estimate beta in
   * XCSoar's wind quality measure (0-5)
   */
  int quality(const ZZBeta& beta, const bool circling) const;

  /**
   * Find item in list with worst squared error
   */
  ObsList::iterator find_worst(const ZZBeta& beta);
};

class WindZigZagGlue: public WindZigZag
{
public:
  SpeedVector optimises(int &res_quality, const SpeedVector start,
                        const bool circling);

  int Update(const NMEA_INFO &basic, const DERIVED_INFO &derived,
             SpeedVector &wind);
};

static WindZigZagGlue wind_zig_zag;

int
WindZigZagUpdate(const NMEA_INFO &basic, const DERIVED_INFO &derived,
                 SpeedVector &wind)
{
  return wind_zig_zag.Update(basic, derived, wind);
}

SpeedVector
WindZigZagGlue::optimises(int &res_quality, const SpeedVector start,
                          const bool circling)
{
  const ZZBeta beta = optimise(res_quality, start, circling);
  SpeedVector wind(beta.east, beta.north);
  return wind;
}

int
WindZigZagGlue::Update(const NMEA_INFO &basic, const DERIVED_INFO &derived,
                       SpeedVector &wind)
{
  // @todo accuracy: correct TAS for vertical speed if dynamic pullup

  // reset if flight hasnt started or airspeed instrument not available
  if (!derived.flight.Flying ||
      !basic.AirspeedAvailable) {
    reset();
    return 0;
  }

  // ensure system is reset if time retreats
  if (back_in_time(basic.Time)) {
    return 0;
  }

  // temporary manoeuvering, dont append this point
  if ((fabs(derived.TurnRate) > fixed(20)) ||
      (fabs(basic.acceleration.Gload - fixed_one) > fixed(0.3))) {

    blackout(basic.Time);
    return 0;
  }

  // is this point able to be added?
  if (!do_append(basic.Time, basic.track))
    return 0;

  // ok to add a point

  append(ZZObs(basic.Time,
               basic.GroundSpeed, basic.track,
               basic.TrueAirspeed));

  //
  if (!full())
    return 0;

  int quality;
  wind = optimises(quality, derived.wind, derived.Circling);
  return quality;
}

/**
 * Find optimal value
 */
WindZigZag::ZZBeta
WindZigZag::optimise(int &res_quality, const SpeedVector start,
                     const bool circling)
{

  fixed sin, cos;
  start.bearing.sin_cos(sin, cos);

  double x[] = {
      sin * start.norm,
      cos * start.norm
  };
  min_newuoa<double, WindZigZag>(2, x, *this, 100.0, 0.0001, 100);

  const ZZBeta beta((fixed)x[0], (fixed)x[1]);
  res_quality = quality(beta, circling);

  std::pair<fixed, fixed> c = correlation(beta);
  prune_worst(beta);

  /*
  if linear correlation coefficient is low, then TAS does not fit est TAS
  across the samples, so fit is poor and should be rejected due to bad
  estimate or bad samples.
  */
  if (c.first < fixed(0.9))
    res_quality = 0;

  return beta;
}

/**
 * Remove items which have high relative error (leaving at least some items).
 * This is to remove outliers which are likely to be caused by manoeuvering.
 * On average, more outliers than useful values are likely to be removed
 * by this algorithm.
 */
void
WindZigZag::prune_worst(const ZZBeta &beta)
{
  if (!obs.empty())
    // always remove one old point, otherwise we may end up rejecting
    // all new points, ending up with a very old data set.
    obs.pop_front();

  fixed relative_error_sqr = relative_error_squared(beta);

  while (size() > 5 && relative_error_sqr > fixed(0.0025))
    remove_worst(beta);
}

std::pair<fixed, fixed>
WindZigZag::correlation(const ZZBeta& beta) const
{
  fixed x_av(0);
  fixed y_av(0);

  unsigned n = 0;
  for (ObsList::const_iterator it = obs.begin(); it != obs.end(); ++it) {
    x_av += it->Phi(beta);
    y_av += it->mag();
    n++;
  }

  if (!n)
    return std::make_pair(fixed_zero, fixed_one);

  x_av /= n;
  y_av /= n;

  fixed acc_xy(0);
  fixed acc_xx(0);
  fixed acc_yy(0);
  for (ObsList::const_iterator it = obs.begin(); it != obs.end(); ++it) {
    fixed xd = it->Phi(beta) - y_av;
    fixed yd = it->mag() - x_av;
    acc_xy += xd * yd;
    acc_xx += xd * xd;
    acc_yy += yd * yd;
  }
  if (!positive(acc_xx) || !positive(acc_yy))
    return std::make_pair(fixed_zero, fixed_one);
  fixed r = acc_xy / (sqrt(acc_xx) * sqrt(acc_yy));
  fixed slope = y_av / x_av;

  return std::make_pair(r, slope);
}

fixed
WindZigZag::fmin(const ZZBeta& beta) const
{
  fixed acc(0);
  for (ObsList::const_iterator it = obs.begin(); it != obs.end(); ++it)
    acc += sqr(it->f(beta) / it->mag());

  return acc;
}

void
WindZigZag::remove_worst(const ZZBeta& beta)
{
  ObsList::iterator i_worst = find_worst(beta);
  if (i_worst != obs.end())
    obs.erase(i_worst);
}

fixed
WindZigZag::relative_error_squared(const ZZBeta& beta) const
{
  return fmin(beta);
}

int
WindZigZag::quality(const ZZBeta& beta, const bool circling) const
{
  const fixed v = sqrt(relative_error_squared(beta));
  const int quality = min(fixed(5.0), max(fixed(1.0), -log(v)));
  if (circling)
    return max(1, quality / 2); // de-value updates in circling mode
  else
    return quality;
}

WindZigZag::ObsList::iterator
WindZigZag::find_worst(const ZZBeta& beta)
{
  fixed worst(0);

  ObsList::iterator i_worst = obs.end();
  for (ObsList::iterator it = obs.begin(); it != obs.end(); ++it) {
    fixed fthis = sqr(it->f(beta) / it->mag());
    if (fthis > worst) {
      i_worst = it;
      worst = fthis;
    }
  }
  return i_worst;
}

bool
WindZigZag::back_in_time(const unsigned time)
{
  if (obs.empty())
    return false;

  if (time < obs.back().time) {
    reset();
    return true;
  }

  return false;
}

bool
WindZigZag::do_append(const unsigned time, const Angle &ang) const
{
  // never add if during maneuvering blackout
  if (time < time_blackout + BLACKOUT_TIME)
    return false;

  // add if empty
  if (!size())
    return true;

  // don't add if same time
  if (time == obs.back().time)
    return false;

  // don't add if no angle spread
  if ((ang - obs.back().track).as_delta().magnitude_degrees() < fixed(10))
    return false;

  // okay to add
  return true;
}

void
WindZigZag::blackout(const unsigned time)
{
  time_blackout = time;
}

void
WindZigZag::append(const ZZObs &o)
{
  if (full())
    obs.pop_front();

  obs.push_back(o);
  time_blackout = (unsigned)-1;
}
