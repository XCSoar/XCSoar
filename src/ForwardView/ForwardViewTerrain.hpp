// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ForwardView/ForwardViewGeometry.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Math/Angle.hpp"
#include "Terrain/Height.hpp"
#include "Terrain/RasterMap.hpp"
#include "Terrain/RasterProjection.hpp"
#include "Terrain/RasterTraits.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ForwardViewTerrain {

/** Forward (+) distance from @p origin to @p point along @p track. */
inline double
AlongTrackDistance(const GeoPoint &origin, const GeoPoint &point,
                   Angle track) noexcept
{
  const GeoVector rel(origin, point);
  return rel.distance * (rel.bearing - track).cos();
}

/** Snap to the centre of a terrain raster cell for stable height lookups. */
inline GeoPoint
SnapHeightSample(const RasterMap &map, GeoPoint point) noexcept
{
  const RasterProjection &proj = map.GetProjection();
  const SignedRasterLocation fine = proj.ProjectFine(point);
  constexpr int shift = int(RasterTraits::SUBPIXEL_BITS);
  const int half = 1 << (shift - 1);
  const SignedRasterLocation snapped{
    ((fine.x >> shift) << shift) + half,
    ((fine.y >> shift) << shift) + half,
  };
  return proj.UnprojectFine(snapped);
}

inline TerrainHeight
HeightAt(const RasterMap &map, GeoPoint point) noexcept
{
  return map.GetHeight(SnapHeightSample(map, point));
}

/** Smooth height for the forward-view mesh (bilinear DEM interpolation). */
inline TerrainHeight
MeshHeightAt(const RasterMap &map, GeoPoint point) noexcept
{
  return map.GetInterpolatedHeight(point);
}

inline double
MeshElevation(double raw_msl, double aircraft_alt, double vertical_ref,
              double x, double y, const TerrainHeight &h) noexcept
{
  if (h.IsInvalid())
    return 0.;

  double msl = raw_msl;
  if (h.IsWater())
    msl = 0.;

  return ForwardViewGeometry::RelativeTerrainZ(msl, aircraft_alt,
                                               vertical_ref, x, y, false);
}

inline GeoPoint
LocalToGeo(const GeoPoint &origin, Angle track,
           double x, double y) noexcept
{
  if (std::abs(x) < 0.01 && std::abs(y) < 0.01)
    return origin;

  const double dist = std::hypot(x, y);
  const Angle bearing = track + Angle::Radians(std::atan2(y, x));
  return GeoVector{dist, bearing}.EndPoint(origin);
}

/** True when no exaggerated terrain along the eye ray blocks the point. */
inline bool
PointVisibleFromEye(const RasterMap &map, const GeoPoint &origin,
                    Angle track, double aircraft_alt, double vertical_ref,
                    double x, double y, double z) noexcept
{
  if (x <= 0.)
    return false;

  const unsigned steps = std::clamp(
    unsigned(std::hypot(x, y) / 150.),
    8u, 48u);
  constexpr double margin = 10.;

  for (unsigned k = 1; k < steps; ++k) {
    const double t = double(k) / double(steps);
    const double px = x * t;
    const double py = y * t;
    const double pz = z * t;

    const GeoPoint geo = LocalToGeo(origin, track, px, py);
    const TerrainHeight h = HeightAt(map, geo);
    if (h.IsSpecial())
      continue;

    const double terrain_z = MeshElevation(h.GetValue(), aircraft_alt,
                                           vertical_ref, px, py, h);
    if (terrain_z > pz + margin)
      return false;
  }

  return true;
}

/** Bilinear height lookup on the cached forward-view mesh (anchor frame). */
inline bool
SampleMeshHeight(double x, double y,
                 const std::vector<double> &xs,
                 const std::vector<std::vector<double>> &ys,
                 const std::vector<std::vector<double>> &heights,
                 unsigned dist_count, unsigned lateral_count,
                 double &out_z) noexcept
{
  if (dist_count < 2 || lateral_count < 2 ||
      xs.empty() || x < xs.front() || x > xs.back())
    return false;

  const auto row_it = std::upper_bound(xs.begin(), xs.end(), x);
  if (row_it == xs.begin() || row_it == xs.end())
    return false;

  const unsigned i1 = unsigned(row_it - xs.begin());
  const unsigned i0 = i1 - 1;
  const double tx = (x - xs[i0]) / (xs[i1] - xs[i0]);

  const auto sample_row = [&](unsigned i, double y_val, double &z) -> bool {
    if (i >= ys.size() || ys[i].size() != lateral_count)
      return false;

    const double y_min = ys[i].front();
    const double y_max = ys[i].back();
    if (y_val < y_min || y_val > y_max)
      return false;

    const double span = y_max - y_min;
    if (span <= 0.01)
      return false;

    const double fj = (y_val - y_min) / span * double(lateral_count - 1);
    const unsigned j0 = std::min(unsigned(fj), lateral_count - 2);
    const double ty = fj - double(j0);
    z = heights[i][j0] * (1. - ty) + heights[i][j0 + 1] * ty;
    return true;
  };

  double z0 = 0., z1 = 0.;
  if (!sample_row(i0, y, z0) || !sample_row(i1, y, z1))
    return false;

  out_z = z0 * (1. - tx) + z1 * tx;
  return true;
}

inline double
MeshMaxLateralExtent(const std::vector<std::vector<double>> &ys) noexcept
{
  double max_hw = 0.;
  for (const auto &row : ys) {
    if (row.empty())
      continue;

    max_hw = std::max(max_hw, std::abs(row.front()));
    max_hw = std::max(max_hw, std::abs(row.back()));
  }

  return max_hw;
}

inline double
MaxShadowRayLength(double x, double y,
                   double dir_x, double dir_y,
                   const std::vector<double> &xs,
                   const std::vector<std::vector<double>> &ys) noexcept
{
  if (xs.empty())
    return 0.;

  const double x_min = xs.front();
  const double x_max = xs.back();
  const double max_hw = MeshMaxLateralExtent(ys);
  const double y_min = -max_hw;
  const double y_max = max_hw;

  double max_t = 1e9;

  if (std::abs(dir_x) > 1e-6) {
    const double tx = dir_x > 0.
      ? (x_max - x) / dir_x
      : (x_min - x) / dir_x;
    if (tx > 0.)
      max_t = std::min(max_t, tx);
  }

  if (std::abs(dir_y) > 1e-6) {
    const double ty = dir_y > 0.
      ? (y_max - y) / dir_y
      : (y_min - y) / dir_y;
    if (ty > 0.)
      max_t = std::min(max_t, ty);
  }

  return std::max(0., max_t);
}

/**
 * True when exaggerated terrain blocks the sun along the light direction.
 * Coordinates are in the mesh anchor frame.
 */
inline bool
PointCastShadow(double x, double y, double z,
                double dir_x, double dir_y, double dir_z,
                const std::vector<double> &xs,
                const std::vector<std::vector<double>> &ys,
                const std::vector<std::vector<double>> &heights,
                unsigned dist_count, unsigned lateral_count,
                double sample_step) noexcept
{
  if (dir_z <= 0.01)
    return false;

  constexpr double bias = 1.5;
  constexpr unsigned max_steps = 256;

  const double step = std::clamp(sample_step * 0.2, 6., 25.);
  const double max_t =
    MaxShadowRayLength(x, y, dir_x, dir_y, xs, ys);

  for (unsigned n = 0, steps = max_steps; n < steps; ++n) {
    const double t = step * double(n + 1);
    if (t > max_t)
      break;

    const double px = x + t * dir_x;
    const double py = y + t * dir_y;
    const double pz = z + t * dir_z;

    double terrain_z = 0.;
    if (!SampleMeshHeight(px, py, xs, ys, heights,
                          dist_count, lateral_count, terrain_z))
      continue;

    if (terrain_z > pz - bias)
      return true;
  }

  return false;
}

} // namespace ForwardViewTerrain
