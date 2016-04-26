/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "RouteLink.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Terrain/RasterMap.hpp"

#define MC_CEILING_PENALTY_FACTOR 5.0

inline FlatGeoPoint
RoutePolars::MSLIntercept(const int index, const FlatGeoPoint &fp,
                          double altitude,
                          const FlatProjection &proj) const
{
  const unsigned safe_index = ((unsigned)index) % ROUTEPOLAR_POINTS;
  const auto d = altitude * polar_glide.GetPoint(safe_index).inv_gradient;
  const auto scale = proj.GetApproximateScale();
  const int steps = int(d / scale) + 1;
  FlatGeoPoint dp = RoutePolar::IndexToDXDY(safe_index);
  dp.x = (dp.x * steps) >> 7;
  dp.y = (dp.y * steps) >> 7;
  return fp + dp;
}

void
RoutePolars::Initialise(const GlideSettings &settings, const GlidePolar &polar,
                        const SpeedVector &wind,
                        const int _height_min_working)
{
  polar_glide.Initialise(settings, polar, wind, true);
  polar_cruise.Initialise(settings, polar, wind, false);
  inv_mc = MC_CEILING_PENALTY_FACTOR * polar.GetInvMC();
  height_min_working = std::max(0, _height_min_working - GetSafetyHeight());
}

unsigned
RoutePolars::RoundTime(const unsigned val)
{
  return val | 0x07;
}

unsigned
RoutePolars::CalcTime(const RouteLink& link) const
{
  const int dh = link.second.altitude - link.first.altitude;
  if (dh < 0 && inv_mc <= 0)
    // impossible, can't climb
    return UINT_MAX;

  const auto &glide_polar_point = polar_glide.GetPoint(link.polar_index);
  const auto &cruise_polar_point = polar_cruise.GetPoint(link.polar_index);

  // dh/d = gradient
  const auto rho = dh > 0
    ? std::min(1., (dh * link.inv_d * glide_polar_point.inv_gradient))
    : 0.;

  if (rho < 1 && !cruise_polar_point.valid)
    // impossible, can't cruise
    return UINT_MAX;

  if (rho > 0 && !glide_polar_point.valid)
    // impossible, can't glide
    return UINT_MAX;

  const int t_cruise = (int)(
    link.d * (rho * glide_polar_point.slowness +
    (1 - rho) * cruise_polar_point.slowness));

  if (link.second.altitude > cruise_altitude) {
    // penalise any climbs required above cruise altitude
    const int h_penalty = std::max(
        0,
        link.second.altitude - std::max(cruise_altitude, link.first.altitude));
    return t_cruise + (int)(h_penalty * inv_mc);
  }

  return t_cruise;
}

double
RoutePolars::CalcVHeight(const RouteLink &link) const
{
  return polar_glide.GetPoint(link.polar_index).gradient * link.d;
}

bool
RoutePolars::CheckClearance(const RouteLink &e, const RasterMap* map,
                            const FlatProjection &proj, RoutePoint& inp) const
{
  if (!config.IsTerrainEnabled())
    return true;

  GeoPoint int_x;
  int int_h;
  GeoPoint start = proj.Unproject(e.first);
  GeoPoint dest = proj.Unproject(e.second);

  assert(map);

  if (!map->FirstIntersection(start, e.first.altitude, dest,
                              e.second.altitude, CalcVHeight(e),
                              climb_ceiling, GetSafetyHeight(),
                              int_x, int_h))
    return true;

  inp = RoutePoint(proj.ProjectInteger(int_x), int_h);
  return false;
}

RouteLink
RoutePolars::GenerateIntermediate(const RoutePoint& _dest,
                                   const RoutePoint& _origin,
                                   const FlatProjection &proj) const
{
  RouteLink link(_dest, _origin, proj);
  const int vh = CalcVHeight(link) + _dest.altitude;
  if (CanClimb())
    link.second.altitude = std::max(_dest.altitude, std::min(vh, cruise_altitude));
  else
    link.second.altitude = vh;
  return link;
}

RouteLink
RoutePolars::NeighbourLink(const RoutePoint &start, const RoutePoint &end,
                           const FlatProjection &proj, const int sign) const
{
  const FlatGeoPoint d = FlatGeoPoint(end) - FlatGeoPoint(start);

  // table of rotations for different maximum lengths.  these are calculated so
  // there is sufficient rotation as lengths get small for deltas to not
  // disappear with rounding.

  // rotation matrix is [c -s
  //                     s c]

  // Table calculations:
  // sina = 256/maxv
  // a = asin(sina/256)
  // cosa = 256*cos(a)

  static constexpr int sina[] =
    {256, 128, 85, 64, 51, 43, 37, 32, 28, 26, 23, 21, 20, 18 };
  static constexpr int cosa[] =
    {256, 222, 241, 248, 251, 252, 253, 254, 254, 255, 255, 255, 255, 255 };

  const int index = std::min((int)8,
                             std::max(abs(d.x), abs(d.y)) - 1);

  FlatGeoPoint dr((d.x * cosa[index] - d.y * sina[index] * sign) >> 8,
                  (d.x * sina[index] * sign + d.y * cosa[index]) >> 8);
  RoutePoint pd(FlatGeoPoint(start) + dr, start.altitude);
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
                       const int _cruise_alt,
                       const int _ceiling_alt)
{
  config = _config;

  cruise_altitude = _cruise_alt;
  if (config.use_ceiling)
    climb_ceiling = std::max(_ceiling_alt, cruise_altitude);
  else
    climb_ceiling = INT_MAX;
}

bool
RoutePolars::CanClimb() const
{
  return config.allow_climb && inv_mc > 0;
}

GeoPoint
RoutePolars::Intersection(const AGeoPoint &origin,
                          const AGeoPoint &destination,
                          const RasterMap *map, const FlatProjection &proj) const
{
  if (map == nullptr || !map->IsDefined())
    return GeoPoint::Invalid();

  RouteLink e(RoutePoint(proj.ProjectInteger(destination),
                         destination.altitude),
              RoutePoint(proj.ProjectInteger(origin), origin.altitude), proj);
  if (e.d <= 0)
    return GeoPoint::Invalid();

  return map->Intersection(origin,
                           origin.altitude - GetSafetyHeight(),
                           CalcVHeight(e), destination,
                           height_min_working);
}

int
RoutePolars::CalcGlideArrival(const AFlatGeoPoint& origin,
                                const FlatGeoPoint& dest,
                              const FlatProjection &proj) const
{
  const RouteLink e(RoutePoint(dest, 0), origin, proj);
  return origin.altitude - CalcVHeight(e);
}

FlatGeoPoint
RoutePolars::ReachIntercept(const int index, const AFlatGeoPoint &flat_origin,
                            const GeoPoint &origin,
                            const RasterMap* map,
                            const FlatProjection &proj) const
{
  const bool valid = map && map->IsDefined();
  const int altitude = flat_origin.altitude - GetSafetyHeight();
  const FlatGeoPoint flat_dest = MSLIntercept(index, flat_origin,
                                              altitude, proj);

  if (!valid)
    return flat_dest;

  const GeoPoint dest = proj.Unproject(flat_dest);
  const GeoPoint p = map->Intersection(origin, altitude,
                                       altitude, dest, height_min_working);

  if (!p.IsValid())
    return flat_dest;

  FlatGeoPoint fp = proj.ProjectInteger(p);

  /* when there's an obstacle very nearby and our intersection is
     right next to our origin, the intersection may be deformed due to
     terrain raster rounding errors; the following code applies
     clipping to avoid degenerate polygons */
  FlatGeoPoint delta1 = flat_dest - (FlatGeoPoint)flat_origin;
  FlatGeoPoint delta2 = fp - (FlatGeoPoint)flat_origin;

  if (delta1.x * delta2.x < 0)
    /* intersection is on the wrong horizontal side */
    fp.x = flat_origin.x;

  if (delta1.y * delta2.y < 0)
    /* intersection is on the wrong vertical side */
    fp.x = flat_origin.y;

  return fp;
}
