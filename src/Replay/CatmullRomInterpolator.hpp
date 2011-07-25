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

#include "Math/fixed.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/Navigation/Geometry/GeoVector.hpp"

#include <algorithm>
#include <assert.h>

/**
 * A Catmull-Rom splines interpolator
 * @see http://www.cs.cmu.edu/~462/projects/assn2/assn2/catmullRom.pdf
 */
class CatmullRomInterpolator
{
public:
  struct Record {
    GeoPoint loc;
    fixed alt;
    fixed palt;
    fixed t;
  };

private:
  const fixed t;

  unsigned num;
  Record p[4];

public:
  CatmullRomInterpolator(fixed _t):
    t(_t)
  {
    Reset();
  }

  void
  Reset()
  {
    num = 0;
  }

  void
  Update(fixed t, GeoPoint location, fixed alt, fixed palt)
  {
    if (num && (t <= p[3].t))
      return;

    if (num < 4)
      num++;

    std::copy(p + 1, p + 4, p);

    p[3].loc = location;
    p[3].alt = alt;
    p[3].palt = palt;
    p[3].t = t;
  }

  bool
  Ready() const
  {
    return (num == 4);
  }

  GeoVector 
  GetVector(fixed time) const
  {
    assert(Ready());

    if (!positive(p[2].t-p[1].t)) {
      return GeoVector(fixed_zero, Angle::zero());
    }

    const Record r0 = Interpolate(time - fixed(0.05));
    const Record r1 = Interpolate(time + fixed(0.05));
    return GeoVector(p[1].loc.distance(p[2].loc)/
                     (p[2].t-p[1].t), r0.loc.bearing(r1.loc));
  }

  gcc_pure
  Record
  Interpolate(fixed time) const
  {
    assert(Ready());

    const fixed u = time_fraction(time, false);

    if (!positive(u))
      return p[1];

    if (u > fixed_one)
      return p[2];

    /*
      ps = ( c0   c1    c2  c3)
           [  0    1     0   0] 1
           [ -t    0     t   0] u
           [ 2t  t-3  3-2t  -t] u^2
           [ -t  2-t   t-2   t] u^3
    */

    const fixed u2 = u * u;
    const fixed u3 = u2 * u;
    const fixed c[4]= {-t * u3 + 2 * t * u2 - t * u,
                       (fixed_two - t) * u3 + (t - fixed(3)) * u2 + fixed_one,
                       (t - fixed_two) * u3 + (fixed(3) - 2 * t) * u2 + t * u,
                        t * u3 - t * u2};

    Record r;
    r.loc.Latitude = p[0].loc.Latitude * c[0] + p[1].loc.Latitude * c[1]
      + p[2].loc.Latitude * c[2] + p[3].loc.Latitude * c[3];

    r.loc.Longitude = p[0].loc.Longitude * c[0] + p[1].loc.Longitude * c[1]
      + p[2].loc.Longitude * c[2] + p[3].loc.Longitude * c[3];

    r.alt = p[0].alt * c[0] + p[1].alt * c[1] +
      p[2].alt * c[2] + p[3].alt * c[3];
    r.palt = p[0].palt * c[0] + p[1].palt * c[1] +
      p[2].palt * c[2] + p[3].palt * c[3];

    r.t = time;

    return r;
  }

  fixed
  GetMinTime() const
  {
    assert(Ready());

    return p[0].t;
  }

  fixed
  GetMaxTime() const
  {
    assert(Ready());

    return max(fixed_zero, max(p[0].t, max(p[1].t, max(p[2].t, p[3].t))));
  }

  bool
  NeedData(fixed t_simulation) const
  {
    return !Ready() || (p[2].t <= t_simulation + fixed(0.1));
  }

private:

  fixed time_fraction(const fixed time, bool limit_range=true) const {
    assert(Ready());
    assert(p[2].t>p[1].t);
    const fixed u = (time - p[1].t) / (p[2].t - p[1].t);
    if (limit_range) {
      return max(fixed_zero, min(fixed_one, u));
    } else {
      return u;
    }
  }
};
