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

#include "RoutePolars.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Terrain/RasterMap.hpp"

#define MC_CEILING_PENALTY_FACTOR 5.0

GeoPoint
RoutePolars::MSLIntercept(const int index, const AGeoPoint& p,
                           const TaskProjection& proj) const
{
  const unsigned safe_index = ((unsigned)index) % ROUTEPOLAR_POINTS;
  const FlatGeoPoint fp = proj.project(p);
  const fixed d = p.altitude * polar_glide.GetPoint(safe_index).inv_gradient;
  const fixed scale = proj.get_approx_scale();
  const int steps = int(d / scale) + 1;
  int dx, dy;
  RoutePolar::IndexToDXDY(safe_index, dx, dy);
  dx = (dx * steps) >> 7;
  dy = (dy * steps) >> 7;
  const FlatGeoPoint dp(fp.Longitude + dx, fp.Latitude + dy);
  return proj.unproject(dp);
}

void
RoutePolars::Initialise(const GlidePolar& polar, const SpeedVector& wind)
{
  polar_glide.Initialise(polar, wind, true);
  polar_cruise.Initialise(polar, wind, false);
  const fixed imc = polar.GetInvMC();
  if (positive(imc))
    inv_mc = fixed(MC_CEILING_PENALTY_FACTOR) * imc;
  else
    inv_mc = fixed_zero;
}

unsigned
RoutePolars::RoundTime(const unsigned val)
{
  return val | 0x07;
}

unsigned
RoutePolars::CalcTime(const RouteLink& link) const
{
  const RoughAltitude dh = link.second.altitude - link.first.altitude;
  if (dh.IsNegative() && !positive(inv_mc))
    // impossible, can't climb
    return UINT_MAX;

  // dh/d = gradient
  const fixed rho = dh.IsPositive() ?
    std::min(fixed_one, (dh * link.inv_d *
                         polar_glide.GetPoint(link.polar_index).inv_gradient)) :
    fixed_zero;

  if ((rho < fixed_one) && !polar_cruise.GetPoint(link.polar_index).valid)
    // impossible, can't cruise
    return UINT_MAX;

  if (positive(rho) && !polar_glide.GetPoint(link.polar_index).valid)
    // impossible, can't glide
    return UINT_MAX;

  const int t_cruise = (int)(
    link.d * (rho * polar_glide.GetPoint(link.polar_index).slowness +
    (fixed_one - rho) * polar_cruise.GetPoint(link.polar_index).slowness));

  if (link.second.altitude > cruise_altitude) {
    // penalise any climbs required above cruise altitude
    const RoughAltitude h_penalty = std::max(
        RoughAltitude(0),
        link.second.altitude - std::max(cruise_altitude, link.first.altitude));
    return t_cruise + (int)(h_penalty * inv_mc);
  }

  return t_cruise;
}

RoughAltitude
RoutePolars::CalcVHeight(const RouteLink &link) const
{
  return RoughAltitude(polar_glide.GetPoint(link.polar_index).gradient * link.d);
}

bool
RoutePolars::CheckClearance(const RouteLink &e, const RasterMap* map,
                             const TaskProjection &proj, RoutePoint& inp) const
{
  if (!config.terrain_enabled())
    return true;

  GeoPoint int_x;
  short int_h;
  GeoPoint start = proj.unproject(e.first);
  GeoPoint dest = proj.unproject(e.second);

  assert(map);

  if (!map->FirstIntersection(start, (short)e.first.altitude, dest,
                              (short)e.second.altitude, (short)CalcVHeight(e),
                              (short)climb_ceiling, (short)GetSafetyHeight(),
                              int_x, int_h))
    return true;

  inp = RoutePoint(proj.project(int_x), RoughAltitude(int_h));
  return false;
}

RouteLink
RoutePolars::GenerateIntermediate(const RoutePoint& _dest,
                                   const RoutePoint& _origin,
                                   const TaskProjection& proj) const
{
  RouteLink link(_dest, _origin, proj);
  const RoughAltitude vh = CalcVHeight(link) + _dest.altitude;
  if (CanClimb())
    link.second.altitude = std::max(_dest.altitude, std::min(vh, cruise_altitude));
  else
    link.second.altitude = vh;
  return link;
}

RouteLink
RoutePolars::NeighbourLink(const RoutePoint &start, const RoutePoint &end,
                            const TaskProjection &proj, const int sign) const
{
  const FlatGeoPoint d = end - start;

  // table of rotations for different maximum lengths.  these are calculated so
  // there is sufficient rotation as lengths get small for deltas to not
  // disappear with rounding.

  // rotation matrix is [c -s
  //                     s c]

  // Table calculations:
  // sina = 256/maxv
  // a = asin(sina/256)
  // cosa = 256*cos(a)

  static gcc_constexpr_data int sina[] =
    {256, 128, 85, 64, 51, 43, 37, 32, 28, 26, 23, 21, 20, 18 };
  static gcc_constexpr_data int cosa[] =
    {256, 222, 241, 248, 251, 252, 253, 254, 254, 255, 255, 255, 255, 255 };

  const int index = std::min((int)8,
                             std::max(abs(d.Longitude), abs(d.Latitude)) - 1);

  FlatGeoPoint dr(
      (d.Longitude * cosa[index] - d.Latitude * sina[index] * sign) >> 8,
      (d.Longitude * sina[index] * sign + d.Latitude * cosa[index]) >> 8);
  RoutePoint pd(start + dr, start.altitude);
  pd.RoundLocation();
  return GenerateIntermediate(start, pd, proj);
}

bool
RoutePolars::IsAchievable(const RouteLink& link, const bool check_ceiling) const
{
  if (CanClimb())
    return true;

  if (check_ceiling &&
      config.use_ceiling &&
      link.second.altitude > climb_ceiling)
    return false;

  return link.second.altitude <= cruise_altitude &&
         link.second.altitude - link.first.altitude >= CalcVHeight(link);
}

void
RoutePolars::SetConfig(const RoutePlannerConfig& _config,
                        const RoughAltitude _cruise_alt,
                        const RoughAltitude _ceiling_alt)
{
  config = _config;

  cruise_altitude = _cruise_alt;
  if (config.use_ceiling)
    climb_ceiling = std::max(_ceiling_alt, cruise_altitude);
  else
    climb_ceiling = SHRT_MAX;
}

bool
RoutePolars::CanClimb() const
{
  return config.allow_climb && positive(inv_mc);
}

bool
RoutePolars::Intersection(const AGeoPoint& origin, const AGeoPoint& destination,
                          const RasterMap* map, const TaskProjection& proj,
                          GeoPoint& intx) const
{
  if (!map || !map->isMapLoaded())
    return false;

  RouteLink e(RoutePoint(proj.project(destination), destination.altitude),
              RoutePoint(proj.project(origin), origin.altitude), proj);
  if (!positive(e.d))
    return false;

  const RoughAltitude h_diff = origin.altitude - destination.altitude;

  if (!h_diff.IsPositive()) {
    // assume gradual climb to destination
    intx = map->Intersection(origin, (short)(origin.altitude - GetSafetyHeight()),
                             (short)h_diff, destination);
    return !(intx == destination);
  }

  const RoughAltitude vh = CalcVHeight(e);

  if (h_diff > vh) {
    // have excess height to glide, scan pure glide, will arrive at destination high

    intx = map->Intersection(origin, (short)(origin.altitude - GetSafetyHeight()),
                             (short)vh, destination);
    return !(intx == destination);
  }

  // mixed cruise-climb then glide segments, do separate searches for each

  // proportion of flight as glide
  const fixed p = h_diff / vh;

  // location of start of glide
  const GeoPoint p_glide = destination.Interpolate(origin, p);

  // intersects during cruise-climb?
  intx = map->Intersection(origin, (short)(origin.altitude - GetSafetyHeight()),
                           0, destination);
  if (!(intx == destination))
    return true;

  // intersects during glide?
  intx = map->Intersection(p_glide, (short)(origin.altitude - GetSafetyHeight()),
                           (short)h_diff, destination);
  return !(intx == destination);
}

RoughAltitude
RoutePolars::CalcGlideArrival(const AFlatGeoPoint& origin,
                                const FlatGeoPoint& dest,
                                const TaskProjection& proj) const
{
  const RouteLink e(RoutePoint(dest, RoughAltitude(0)), origin, proj);
  return origin.altitude - CalcVHeight(e);
}

FlatGeoPoint
RoutePolars::ReachIntercept(const int index, const AGeoPoint& origin,
                             const RasterMap* map,
                             const TaskProjection& proj) const
{
  const bool valid = map && map->isMapLoaded();
  const RoughAltitude altitude = origin.altitude - GetSafetyHeight();
  const AGeoPoint m_origin((GeoPoint)origin, altitude);
  const GeoPoint dest = MSLIntercept(index, m_origin, proj);
  const GeoPoint p = valid ?
    map->Intersection(m_origin, (short)altitude, (short)altitude, dest) : dest;
  return proj.project(p);
}
