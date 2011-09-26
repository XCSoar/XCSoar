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

/* This library was originally imported from Cumulus
   http://kflog.org/cumulus/ */

#include "Wind/CirclingWind.hpp"
#include "Math/Constants.h"
#include "Math/FastMath.h"
#include "LogFile.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

#include <stdlib.h>
#include <algorithm>

using std::min;

/*
About Windanalysation

Currently, the wind is being analyzed by finding the minimum and the maximum
groundspeeds measured while flying a circle. The direction of the wind is taken
to be the direction in wich the speed reaches it's maximum value, the speed
is half the difference between the maximum and the minimum speeds measured.
A quality parameter, based on the number of circles allready flown (the first
circles are taken to be less accurate) and the angle between the headings at
minimum and maximum speeds, is calculated in order to be able to weigh the
resulting measurement.

There are other options for determining the windspeed. You could for instance
add all the vectors in a circle, and take the resuling vector as the windspeed.
This is a more complex method, but because it is based on more heading/speed
measurements by the GPS, it is probably more accurate. If equiped with
instruments that pass along airspeed, the calculations can be compensated for
changes in airspeed, resulting in better measurements. We are now assuming
the pilot flies in perfect circles with constant airspeed, wich is of course
not a safe assumption.
The quality indication we are calculation can also be approched differently,
by calculating how constant the speed in the circle would be if corrected for
the windspeed we just derived. The more constant, the better. This is again
more CPU intensive, but may produce better results.

Some of the errors made here will be averaged-out by the WindStore, wich keeps
a number of windmeasurements and calculates a weighted average based on quality.
*/

void
CirclingWind::reset()
{
  circleCount = 0;
  circleLeft = false;
  active = false;
  circleDeg = 0;
  last_track = Angle::zero();
  pastHalfway = false;
  curModeOK = false;
  first = true;
}

CirclingWind::Result
CirclingWind::NewSample(const MoreData &info)
{
  if (!active)
    // only work if we are in active mode
    return Result(0);

  Vector curVector;

  bool fullCircle = false;

  // Circle detection
  int diff = (int)(info.track - last_track).as_delta().magnitude_degrees();
  circleDeg += diff;
  last_track = info.track;

  if (circleDeg >= 360) {
    //full circle made!

    fullCircle = true;
    circleDeg = 0;
    circleCount++; //increase the number of circles flown (used
    //to determine the quality)
  }

  curVector = Vector(SpeedVector(info.track, info.ground_speed));

  if (!windsamples.full()) {
    Sample &sample = windsamples.append();
    sample.v = curVector;
    sample.t = info.time;
    sample.mag = info.ground_speed;
  } else {
    // TODO code: give error, too many wind samples
    // or use circular buffer
  }

  if (first || (info.ground_speed < minVector.Magnitude()))
    minVector = curVector;

  if (first || (info.ground_speed > maxVector.Magnitude()))
    maxVector = curVector;

  Result result(0);
  if (fullCircle) { //we have completed a full circle!
    if (!windsamples.full())
      // calculate the wind for this circle, only if it is valid
      result = _calcWind();

    fullCircle = false;

    // should set each vector to average

    minVector = maxVector = Vector((maxVector.x - minVector.x) / 2,
                                   (maxVector.y - minVector.y) / 2);

    first = true;
    windsamples.clear();

    if (startcircle > 1)
      startcircle--;

    if (startcircle == 1) {
      climbstartpos = GeoPoint(info.location.longitude,
                               info.location.latitude);
      climbstarttime = info.time;
      startcircle = 0;
    }

    climbendpos = GeoPoint(info.location.longitude,
                           info.location.latitude);
    climbendtime = info.time;

    //no need to reset fullCircle, it will automaticly be reset in the next itteration.
  }

  first = false;

  return result;
}

void
CirclingWind::slot_newFlightMode(const DerivedInfo &derived,
                                 bool left, int marker)
{
  // we are inactive by default
  active = false;

  // reset the circlecounter for each flightmode
  // change. The important thing to measure is the
  // number of turns in this thermal only.
  circleCount = 0;

  // ignore first two circles in thermal drift calcs
  startcircle = 3;

  circleDeg = 0;
  curModeOK = derived.circling;

  // we are not circling? Exit function!
  if (!curModeOK)
    return;

  circleLeft = left;

  // initialize analyser-parameters
  active = true;
  first = true;
  windsamples.clear();
}

CirclingWind::Result
CirclingWind::_calcWind()
{
  if (windsamples.empty())
    return Result(0);

  // reject if average time step greater than 2.0 seconds
  if ((windsamples.last().t - windsamples[0].t) / (windsamples.size() - 1) > fixed_two)
    return Result(0);

  // find average
  fixed av = fixed_zero;
  for (unsigned i = 0; i < windsamples.size(); i++)
    av += windsamples[i].mag;

  av /= windsamples.size();

  // find zero time for times above average
  fixed rthisp;
  int ithis = 0;
  fixed rthismax = fixed_zero;
  fixed rthismin = fixed_zero;
  int jmax = -1;
  int jmin = -1;

  for (unsigned j = 0; j < windsamples.size(); j++) {
    rthisp = fixed_zero;

    for (unsigned i = 0; i < windsamples.size(); i++) {
      if (i == j)
        continue;

      ithis = (i + j) % windsamples.size();
      unsigned idiff = i;

      if (idiff > windsamples.size() / 2)
        idiff = windsamples.size() - idiff;

      rthisp += (windsamples[ithis].mag) * idiff;
    }

    if ((rthisp < rthismax) || (jmax == -1)) {
      rthismax = rthisp;
      jmax = j;
    }

    if ((rthisp > rthismin) || (jmin == -1)) {
      rthismin = rthisp;
      jmin = j;
    }
  }

  // jmax is the point where most wind samples are below
  // jmin is the point where most wind samples are above

  maxVector = windsamples[jmax].v;
  minVector = windsamples[jmin].v;

  // attempt to fit cycloid

  fixed mag = half(windsamples[jmax].mag - windsamples[jmin].mag);
  fixed rthis = fixed_zero;

  for (unsigned i = 0; i < windsamples.size(); i++) {
    fixed wx, wy;
    ::sin_cos(((i + jmax) % windsamples.size()) * fixed_two_pi / windsamples.size(), &wy, &wx);
    wx = wx * av + mag;
    wy *= av;
    fixed cmag = hypot(wx, wy) - windsamples[i].mag;
    rthis += cmag * cmag;
  }

  rthis /= windsamples.size();
  rthis = sqrt(rthis);

  int quality;

  if (mag > fixed_one)
    quality = 5 - iround(rthis / mag * 3);
  else
    quality = 5 - iround(rthis);

  if (circleCount < 3)
    quality--;
  if (circleCount < 2)
    quality--;
  if (circleCount < 1)
    return Result(0);

  quality = min(quality, 5); //5 is maximum quality, make sure we honour that.

  if (quality < 1)
    //measurment quality too low
    return Result(0);

  Vector a(-mag * maxVector.x / windsamples[jmax].mag,
           -mag * maxVector.y / windsamples[jmax].mag);

  if (a.SquareMagnitude() >= fixed(30 * 30))
    // limit to reasonable values (60 knots), reject otherwise
    return Result(0);

  return Result(quality, a);
}
