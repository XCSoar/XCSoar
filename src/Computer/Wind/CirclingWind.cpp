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
  active = false;
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

  const bool previous_track_available = last_track_available;

  last_track_available = info.track_available;
  last_ground_speed_available = info.ground_speed_available;

  // Circle detection
  if (previous_track_available)
    current_circle += (info.track - last_track).AsDelta().Absolute();

  last_track = info.track;

  const bool fullCircle = current_circle >= Angle::FullCircle();
  if (fullCircle) {
    //full circle made!

    current_circle -= Angle::FullCircle();
    circle_count++; //increase the number of circles flown (used
    //to determine the quality)
  }

  if (!samples.full()) {
    Sample &sample = samples.append();
    sample.time = info.clock;
    sample.vector = SpeedVector(info.track, info.ground_speed);
  } else {
    // TODO code: give error, too many wind samples
    // or use circular buffer
  }

  Result result(0);
  if (fullCircle) { //we have completed a full circle!
    if (!samples.full())
      // calculate the wind for this circle, only if it is valid
      result = CalcWind();

    samples.clear();
  }

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

  current_circle = Angle::Zero();

  // we are not circling? Exit function!
  if (!derived.circling)
    return;

  // initialize analyser-parameters
  active = true;
  last_track_available.Clear();
  last_ground_speed_available.Clear();
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
    av += samples[i].vector.norm;

  av /= samples.size();

  // find zero time for times above average
  fixed rthismax = fixed(0);
  fixed rthismin = fixed(0);
  int jmax = -1;
  int jmin = -1;

  for (unsigned j = 0; j < samples.size(); j++) {
    fixed rthisp = fixed(0);

    for (unsigned i = 0; i < samples.size(); i++) {
      if (i == j)
        continue;

      const unsigned ithis = (i + j) % samples.size();
      unsigned idiff = i;

      if (idiff > samples.size() / 2)
        idiff = samples.size() - idiff;

      rthisp += samples[ithis].vector.norm * idiff;
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

  // attempt to fit cycloid

  fixed mag = Half(samples[jmax].vector.norm - samples[jmin].vector.norm);
  fixed rthis = fixed(0);

  for (const Sample &sample : samples) {
    const auto sc = sample.vector.bearing.SinCos();
    fixed wx = sc.second, wy = sc.first;
    wx = wx * av + mag;
    wy *= av;
    fixed cmag = SmallHypot(wx, wy) - sample.vector.norm;
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

  if (mag >= fixed(30))
    // limit to reasonable values (60 knots), reject otherwise
    return Result(0);

  // jmax is the point where most wind samples are below
  SpeedVector wind(samples[jmax].vector.bearing, mag);
  if (negative(wind.norm))
    wind.norm = -wind.norm;
  else
    wind.bearing = wind.bearing.Reciprocal();

  return Result(quality, wind);
}
