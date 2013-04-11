/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "CirclingWind.hpp"
#include "Math/Constants.h"
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
CirclingWind::Reset()
{
  last_track_available.Clear();
  last_ground_speed_available.Clear();
  circle_count = 0;
  active = false;
  circle_deg = 0;
  last_track = Angle::Zero();
  first = true;
}

CirclingWind::Result
CirclingWind::NewSample(const MoreData &info)
{
  if (!active)
    // only work if we are in active mode
    return Result(0);

  if (last_track_available.FixTimeWarp(info.track_available) ||
      last_ground_speed_available.FixTimeWarp(info.ground_speed_available))
    /* time warp: start from scratch */
    Reset();

  if (!info.track_available.Modified(last_track_available) ||
      !info.ground_speed_available.Modified(last_ground_speed_available))
    /* no updated GPS fix */
    return Result(0);

  last_track_available = info.track_available;
  last_ground_speed_available = info.ground_speed_available;

  Vector curVector;

  // Circle detection
  int diff = (int)(info.track - last_track).AsDelta().AbsoluteDegrees();
  circle_deg += diff;
  last_track = info.track;

  const bool fullCircle = circle_deg >= 360;
  if (fullCircle) {
    //full circle made!

    circle_deg = 0;
    circle_count++; //increase the number of circles flown (used
    //to determine the quality)
  }

  curVector = Vector(SpeedVector(info.track, info.ground_speed));

  if (!samples.full()) {
    Sample &sample = samples.append();
    sample.v = curVector;
    sample.time = info.clock;
    sample.mag = info.ground_speed;
  } else {
    // TODO code: give error, too many wind samples
    // or use circular buffer
  }

  if (first || (info.ground_speed < min_vector.Magnitude()))
    min_vector = curVector;

  if (first || (info.ground_speed > max_vector.Magnitude()))
    max_vector = curVector;

  Result result(0);
  if (fullCircle) { //we have completed a full circle!
    if (!samples.full())
      // calculate the wind for this circle, only if it is valid
      result = CalcWind();

    // should set each vector to average

    min_vector = max_vector = Vector((max_vector.x - min_vector.x) / 2,
                                   (max_vector.y - min_vector.y) / 2);

    first = true;
    samples.clear();
  }

  first = false;

  return result;
}

void
CirclingWind::NewFlightMode(const DerivedInfo &derived)
{
  // we are inactive by default
  active = false;

  // reset the circlecounter for each flightmode
  // change. The important thing to measure is the
  // number of turns in this thermal only.
  circle_count = 0;

  circle_deg = 0;

  // we are not circling? Exit function!
  if (!derived.circling)
    return;

  // initialize analyser-parameters
  active = true;
  first = true;
  samples.clear();
}

CirclingWind::Result
CirclingWind::CalcWind()
{
  if (samples.empty())
    return Result(0);

  // reject if average time step greater than 2.0 seconds
  if ((samples.last().time - samples[0].time) / (samples.size() - 1) > fixed(2))
    return Result(0);

  // find average
  fixed av = fixed(0);
  for (unsigned i = 0; i < samples.size(); i++)
    av += samples[i].mag;

  av /= samples.size();

  // find zero time for times above average
  fixed rthisp;
  int ithis = 0;
  fixed rthismax = fixed(0);
  fixed rthismin = fixed(0);
  int jmax = -1;
  int jmin = -1;

  for (unsigned j = 0; j < samples.size(); j++) {
    rthisp = fixed(0);

    for (unsigned i = 0; i < samples.size(); i++) {
      if (i == j)
        continue;

      ithis = (i + j) % samples.size();
      unsigned idiff = i;

      if (idiff > samples.size() / 2)
        idiff = samples.size() - idiff;

      rthisp += (samples[ithis].mag) * idiff;
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
  max_vector = samples[jmax].v;

  // jmin is the point where most wind samples are above
  min_vector = samples[jmin].v;

  // attempt to fit cycloid

  fixed mag = Half(samples[jmax].mag - samples[jmin].mag);
  fixed rthis = fixed(0);

  const Angle step = Angle::FullCircle() / samples.size();
  Angle angle = step * jmax;
  for (unsigned i = 0; i < samples.size(); i++, angle += step) {
    const auto sc = angle.SinCos();
    fixed wx = sc.second, wy = sc.first;
    wx = wx * av + mag;
    wy *= av;
    fixed cmag = SmallHypot(wx, wy) - samples[i].mag;
    rthis += sqr(cmag);
  }

  rthis /= samples.size();
  rthis = sqrt(rthis);

  int quality;

  if (mag > fixed(1))
    quality = 5 - iround(rthis / mag * 3);
  else
    quality = 5 - iround(rthis);

  if (circle_count < 3)
    quality--;
  if (circle_count < 2)
    quality--;
  if (circle_count < 1)
    return Result(0);

  quality = min(quality, 5); //5 is maximum quality, make sure we honour that.

  if (quality < 1)
    //measurment quality too low
    return Result(0);

  Vector a(-mag * max_vector.x / samples[jmax].mag,
           -mag * max_vector.y / samples[jmax].mag);

  if (a.SquareMagnitude() >= fixed(30 * 30))
    // limit to reasonable values (60 knots), reject otherwise
    return Result(0);

  return Result(quality, a);
}
