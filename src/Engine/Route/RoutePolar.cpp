/* Copyright_License {

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

#include "RoutePolar.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "Geo/SpeedVector.hpp"

#include <assert.h>

gcc_const
static Angle
IndexToAngle(unsigned i)
{
  assert(i < ROUTEPOLAR_POINTS);

  return Angle::QuarterCircle()
    - Angle::FullCircle() * (fixed(i) / ROUTEPOLAR_POINTS);
}

GlideResult
RoutePolar::SolveTask(const GlideSettings &settings,
                      const GlidePolar& glide_polar,
                       const SpeedVector& wind,
                       const Angle theta, const bool glide) const
{
  const MacCready mac_cready(settings, glide_polar);
  GlideState task(GeoVector(fixed(1), theta), fixed(0), fixed(0), wind);
  return glide
    ? mac_cready.SolveStraight(task)
    : mac_cready.Solve(task);
}

void
RoutePolar::Initialise(const GlideSettings &settings, const GlidePolar& polar,
                       const SpeedVector& wind,
                       const bool is_glide)
{
  for (unsigned i = 0; i < ROUTEPOLAR_POINTS; ++i) {
    const Angle ang = IndexToAngle(i);
    GlideResult res = SolveTask(settings, polar, wind, ang, is_glide);
    if (res.IsOk()) {
      RoutePolarPoint point(res.time_elapsed, res.height_glide);
      points[i] = point;
    } else
      points[i].valid = false;
  }
}

void
RoutePolar::IndexToDXDY(const int index, int& dx, int& dy)
{
  static constexpr int sx[ROUTEPOLAR_POINTS]= {
    128, 126, 123, 118, 111, 102, 91, 79, 66, 51, 36, 20, 4, -12, -28, -44,
    -59, -73, -86, -97, -107, -115, -121, -125, -127, -127, -125, -121, -115,
    -107, -97, -86, -73, -59, -44, -28, -12, 4, 20, 36, 51, 66, 79, 91, 102,
    111, 118, 123, 126,
  };
  static constexpr int sy[ROUTEPOLAR_POINTS]= {
    0, 16, 32, 48, 62, 76, 89, 100, 109, 117, 122, 126, 127, 127, 124, 120,
    113, 104, 94, 82, 69, 55, 40, 24, 8, -8, -24, -40, -55, -69, -82, -94,
    -104, -113, -120, -124, -127, -127, -126, -122, -117, -109, -100, -89,
    -76, -62, -48, -32, -16,
  };

  dx = sx[index];
  dy = sy[index];
}
