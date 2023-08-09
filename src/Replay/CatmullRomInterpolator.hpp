// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Util.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "time/Stamp.hpp"

#include <algorithm>
#include <cassert>

/**
 * A Catmull-Rom splines interpolator
 * @see http://www.cs.cmu.edu/~462/projects/assn2/assn2/catmullRom.pdf
 */
class CatmullRomInterpolator
{
public:
  struct Record {
    GeoPoint location;
    double gps_altitude;
    double baro_altitude;
    TimeStamp time;
  };

private:
  const FloatDuration time;

  unsigned num;
  Record p[4];

public:
  explicit CatmullRomInterpolator(FloatDuration _time) noexcept
    :time(_time)
  {
    Reset();
  }

  void
  Reset()
  {
    num = 0;
  }

  void
  Update(TimeStamp t, GeoPoint location, double alt, double palt)
  {
    if (num && (t <= p[3].time))
      return;

    if (num < 4)
      num++;

    std::copy(p + 1, p + 4, p);

    p[3].location = location;
    p[3].gps_altitude = alt;
    p[3].baro_altitude = palt;
    p[3].time = t;
  }

  bool
  Ready() const
  {
    return (num == 4);
  }

  GeoVector 
  GetVector(TimeStamp _time) const noexcept
  {
    assert(Ready());

    if (p[2].time <= p[1].time)
      return GeoVector(0, Angle::Zero());

    const Record r0 = Interpolate(_time - FloatDuration{0.05});
    const Record r1 = Interpolate(_time + FloatDuration{0.05});

    auto speed = p[1].location.DistanceS(p[2].location) / (p[2].time - p[1].time).count();
    Angle bearing = r0.location.Bearing(r1.location);

    return GeoVector(speed, bearing);
  }

  [[gnu::pure]]
  Record
  Interpolate(TimeStamp _time) const noexcept
  {
    assert(Ready());

    const auto u = GetTimeFraction(_time, false);

    /*
      ps = ( c0   c1    c2  c3)
           [  0    1     0   0] 1
           [ -t    0     t   0] u
           [ 2t  t-3  3-2t  -t] u^2
           [ -t  2-t   t-2   t] u^3
    */

    const auto u2 = Square(u);
    const auto u3 = u2 * u;
    const double c[4] = {
      -time.count() * u3 + 2 * time.count() * u2 - time.count() * u,
      (2 - time.count()) * u3 + (time.count() - 3) * u2 + 1,
      (time.count() - 2) * u3 + (3 - 2 * time.count()) * u2 + time.count() * u,
      time.count() * u3 - time.count() * u2,
    };

    Record r;
    r.location.latitude =
        p[0].location.latitude * c[0] + p[1].location.latitude * c[1] +
        p[2].location.latitude * c[2] + p[3].location.latitude * c[3];

    r.location.longitude =
        p[0].location.longitude * c[0] + p[1].location.longitude * c[1] +
        p[2].location.longitude * c[2] + p[3].location.longitude * c[3];

    r.gps_altitude =
        p[0].gps_altitude * c[0] + p[1].gps_altitude * c[1] +
        p[2].gps_altitude * c[2] + p[3].gps_altitude * c[3];

    r.baro_altitude =
        p[0].baro_altitude * c[0] + p[1].baro_altitude * c[1] +
        p[2].baro_altitude * c[2] + p[3].baro_altitude * c[3];

    r.time = _time;

    return r;
  }

  TimeStamp GetMinTime() const noexcept {
    assert(Ready());

    return p[0].time;
  }

  TimeStamp GetMaxTime() const noexcept {
    assert(Ready());

    return std::max({TimeStamp{}, p[0].time, p[1].time, p[2].time, p[3].time});
  }

  bool
  NeedData(TimeStamp t_simulation) const
  {
    return !Ready() || (p[2].time <= t_simulation + FloatDuration{0.1});
  }

private:
  double
  GetTimeFraction(const TimeStamp time,
                  bool limit_range = true) const noexcept
  {
    assert(Ready());
    assert(p[2].time > p[1].time);

    const auto fraction = (time - p[1].time) / (p[2].time - p[1].time);

    if (limit_range)
      return std::clamp(fraction, 0., 1.);
    else
      return fraction;
  }
};
