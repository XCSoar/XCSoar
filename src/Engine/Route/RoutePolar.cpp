/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include "GlideSolvers/GlidePolar.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "Navigation/SpeedVector.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Math/FastMath.h"
#include "Terrain/RasterMap.hpp"
#include <assert.h>
#include <limits.h>

#define MC_CEILING_PENALTY_FACTOR 5.0

GlideResult
RoutePolar::solve_task(const GlidePolar& glide_polar,
                       const SpeedVector& wind,
                       const Angle theta, const bool glide) const
{
  fixed altitude = glide? fixed(1.0e5): fixed_zero;
  GlideState task(GeoVector(fixed(1.0), theta), fixed_zero, altitude, wind);
  return glide_polar.solve(task);
}


void RoutePolar::initialise(const GlidePolar& polar,
                            const SpeedVector& wind,
                            const bool is_glide)
{
  for (unsigned i=0; i< ROUTEPOLAR_POINTS; ++i) {
    Angle ang(Angle::radians(fixed_half_pi-i*fixed_two_pi/ROUTEPOLAR_POINTS));
    GlideResult res = solve_task(polar, wind, ang, is_glide);
    RoutePolarPoint point(res.TimeElapsed, res.HeightGlide);
    if (res.Solution != GlideResult::RESULT_OK)
      point.valid = false;
    points[i] = point;
  }
}


int
RoutePolar::dxdy_to_index(const int dx, const int dy)
{
  const int adx = abs(dx);
  const int ady = abs(dy);
  const int v = i_normalise_sine(adx, ady);
  int index;
  if (adx<= ady) {
    index = v;
  } else {
    index = ROUTEPOLAR_Q1 - v;
  }
  if (dx<0)
    if (dy<0) {
      return index + ROUTEPOLAR_Q2;
    } else {
      return ROUTEPOLAR_Q2-index;
    }
  else if (dy<0)
    return ROUTEPOLAR_Q3-index;

  return index;
}


RoutePolars::RoutePolars(const GlidePolar& polar,
                         const SpeedVector& wind)
{
  initialise(polar, wind);
}


void
RoutePolars::initialise(const GlidePolar& polar,
                        const SpeedVector& wind)
{
  polar_glide.initialise(polar, wind, true);
  polar_cruise.initialise(polar, wind, false);
  const fixed &mc = polar.get_mc();
  if (positive(mc)) {
    inv_M = fixed(MC_CEILING_PENALTY_FACTOR)/mc;
  } else {
    inv_M = fixed_zero;
  }
}


RouteLink::RouteLink (const RouteLinkBase& _link,
                      const TaskProjection &proj):
  RouteLinkBase(_link)
{
  calc_speedups(proj);
}

RouteLink::RouteLink (const RoutePoint& _destination,
                      const RoutePoint& _origin,
                      const TaskProjection &proj):
  RouteLinkBase(_destination, _origin)
{
  calc_speedups(proj);
}

void
RouteLink::calc_speedups(const TaskProjection& proj)
{
  const fixed scale = proj.get_approx_scale();
  const fixed dx = fixed(second.Longitude-first.Longitude);
  const fixed dy = fixed(second.Latitude-first.Latitude);
  mag_rmag(dx, dy, d, inv_d);
  d*= scale;
  inv_d/= scale;
  assert(!negative(d));
  polar_index = RoutePolar::dxdy_to_index(dx, dy);
}


unsigned
RoutePolars::calc_time(const RouteLink& link) const
{
  const int dh = link.second.altitude-link.first.altitude;
  if ((dh<0) && !positive(inv_M))
    return UINT_MAX; // impossible, can't climb

  // dh/d = gradient
  const fixed rho = (dh>0)?
    std::min(fixed_one, (dh*link.inv_d*polar_glide.get_point(link.polar_index).inv_gradient))
    : fixed_zero;

  if ((rho< fixed_one) && !polar_cruise.get_point(link.polar_index).valid)
    return UINT_MAX; // impossible, can't cruise
  if (positive(rho) && !polar_glide.get_point(link.polar_index).valid)
    return UINT_MAX; // impossible, can't glide

  const int t_cruise = (int)(link.d*(rho*polar_glide.get_point(link.polar_index).slowness+
                                     (fixed_one-rho)*polar_cruise.get_point(link.polar_index).slowness));

  if (link.second.altitude > cruise_altitude) {
    // penalise any climbs required above cruise altitude
    const int h_penalty = std::max(0, link.second.altitude-std::max(cruise_altitude, link.first.altitude));
    return t_cruise+(int)(h_penalty*inv_M);
  } else {
    return t_cruise;
  }
}

short
RoutePolars::calc_vheight(const RouteLink &link) const
{
  return iround(polar_glide.get_point(link.polar_index).gradient * link.d);
}

bool
RoutePolars::check_clearance(const RouteLink &e, const RasterMap& map,
                             const TaskProjection &proj,
                             RoutePoint& inp) const
{
  if (!config.terrain_enabled())
    return true;

  GeoPoint int_x;
  short int_h;
  GeoPoint start = proj.unproject(e.first);
  GeoPoint dest = proj.unproject(e.second);

  if (!map.FirstIntersection(start, e.first.altitude,
                             dest, e.second.altitude,
                             calc_vheight(e), climb_ceiling,
                             int_x, int_h))
    return true;

  inp = RoutePoint(proj.project(int_x), int_h);
  return false;
}




RouteLink
RoutePolars::generate_intermediate (const RoutePoint& _dest,
                                    const RoutePoint& _origin,
                                    const TaskProjection& proj) const
{
  RouteLink link(_dest, _origin, proj);
  const short vh = calc_vheight(link)+_dest.altitude;
  if (can_climb())
    link.second.altitude = std::max(_dest.altitude, std::min(vh, cruise_altitude));
  else
    link.second.altitude = vh;
  return link;
}

RouteLink
RoutePolars::neighbour_link(const RoutePoint &start,
                            const RoutePoint &end,
                            const TaskProjection &proj,
                            const int sign) const
{
  const FlatGeoPoint d = end-start;

  // table of rotations for different maximum lengths.  these are calculated so
  // there is sufficient rotation as lengths get small for deltas to not
  // disappear with rounding.

  // rotation matrix is [c -s
  //                     s c]

  // Table calculations:
  // sina = 256/maxv
  // a = asin(sina/256)
  // cosa = 256*cos(a)

  static const int sina[] =
    {256, 128, 85, 64, 51, 43, 37, 32, 28, 26, 23, 21, 20, 18 };
  static const int cosa[] =
    {256, 222, 241, 248, 251, 252, 253, 254, 254, 255, 255, 255, 255, 255 };

  const int index = std::min((int)8, std::max(abs(d.Longitude), abs(d.Latitude))-1);

  FlatGeoPoint dr((d.Longitude * cosa[index] - d.Latitude * sina[index] * sign)>>8,
                  (d.Longitude * sina[index] * sign + d.Latitude * cosa[index])>>8);
  RoutePoint pd(start+dr,
                start.altitude);
  pd.round_location();
  return generate_intermediate(start, pd, proj);
}


void
AFlatGeoPoint::round_location()
{
  // round point to correspond roughly with terrain step size
  Longitude = (Longitude>>2)<<2;
  Latitude = (Latitude>>2)<<2;
}

bool
RoutePolars::achievable(const RouteLink& link, const bool check_ceiling) const
{
  if (can_climb())
    return true;
  if (check_ceiling && config.use_ceiling &&
      (link.second.altitude > climb_ceiling))
    return false;
  return (link.second.altitude <= cruise_altitude)
    && (link.second.altitude-link.first.altitude >= calc_vheight(link));
}

RouteLink
RouteLink::flat() const
{
  RouteLink copy(*this);
  copy.second.altitude = copy.first.altitude;
  return copy;
}

#define ROUTE_MIN_STEP 3

bool
RouteLinkBase::is_short() const
{
  return (abs(first.Longitude-second.Longitude)<ROUTE_MIN_STEP)
    && (abs(first.Latitude-second.Latitude)<ROUTE_MIN_STEP);
}

void
RoutePolars::set_config(const RoutePlannerConfig& _config,
                        const short _cruise_alt,
                        const short _ceiling_alt)
{
  config = _config;

  cruise_altitude = _cruise_alt;
  if (config.use_ceiling) {
    climb_ceiling = std::max(_ceiling_alt, cruise_altitude);
  } else {
    climb_ceiling = SHRT_MAX;
  }
}

bool
RoutePolars::can_climb() const {
  return config.allow_climb && positive(inv_M);
}
