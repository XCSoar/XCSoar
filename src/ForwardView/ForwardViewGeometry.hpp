// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/FAISphere.hpp"
#include "Math/Angle.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace ForwardViewGeometry {

static constexpr double CAMERA_PITCH_DEG = 11.;
static constexpr double CAMERA_FOV_DEG = 65.;
static constexpr double FRUSTUM_MARGIN = 1.12;
static constexpr double MIN_TERRAIN_DIST = 250.;
/** Minimum eye height above local terrain (avoids mesh clipping on the ground). */
static constexpr double CAMERA_EYE_CLEARANCE = 5.;
/** Minimum eye height above the nav-altitude reference (m). */
static constexpr double CAMERA_EYE_MIN = 2.;
/** Perspective near clip (m); keep small for close ground without clipping the eye. */
static constexpr float CAMERA_NEAR_CLIP = 5.f;
/** Perspective far clip and sky dome radius (m). */
static constexpr float CAMERA_FAR_CLIP = 80000.f;

/** Default along-track terrain/topography span (m). */
static constexpr double VIEW_RANGE_DEFAULT = 40000.;
static constexpr double VIEW_RANGE_MIN = 5000.;
static constexpr double VIEW_RANGE_MAX = double(CAMERA_FAR_CLIP);

/** Experimental smooth camera advance between GPS fixes. */
static constexpr bool SMOOTH_MOTION_ENABLED = true;
static constexpr std::chrono::milliseconds SMOOTH_MOTION_INTERVAL{100};
/** Cap dead-reckoning ahead of the last fix (s). */
static constexpr double SMOOTH_MOTION_MAX_EXTRAPOLATION = 2.;
/** Minimum ground travel (m) before scheduling another repaint. */
static constexpr double SMOOTH_MOTION_REPAINT_DIST = 6.;
/** Scale terrain relief around @p vertical_ref (1 = true height above aircraft). */
static constexpr double VERTICAL_EXAGGERATION = 1.;
static constexpr double TOPO_SURFACE_OFFSET = 2.;
/** Extra lift for filled polygons above the terrain/water surface. */
static constexpr double TOPO_FILL_OFFSET = 8.;

/**
 * Elevation-ramp weight when the topology texture is draped (1 = full ramp,
 * 0 = neutral land only). Keeps hillshade relief while letting topo colours
 * dominate.
 */
static constexpr float TERRAIN_RAMP_TOPO_WEIGHT = 1.f;

/** Upper bounds when the view span exceeds practical DEM resolution. */
static constexpr unsigned MAX_TERRAIN_DIST_SAMPLES = 384;
static constexpr unsigned MAX_TERRAIN_LATERAL_SAMPLES = 96;

/** Clamp a reported DEM cell size to a sane range (m). */
inline double
ClampDemCellSpacing(double cell_spacing) noexcept
{
  return std::clamp(cell_spacing, 5., 500.);
}

/** Mesh rows along track; target ~1.5× DEM cell spacing for smooth silhouettes. */
inline unsigned
DistSampleCount(double range, double cell_spacing) noexcept
{
  const double spacing = cell_spacing * 1.5;
  const double span = std::max(range - MIN_TERRAIN_DIST, spacing);
  const unsigned wanted = unsigned(std::ceil(span / spacing)) + 1;
  return std::clamp(wanted, 2u, MAX_TERRAIN_DIST_SAMPLES);
}

/** Mesh columns across track for the widest row in the frustum. */
inline unsigned
LateralSampleCount(double half_width, double cell_spacing) noexcept
{
  const double spacing = cell_spacing * 1.5;
  const double span = std::max(2. * half_width, spacing);
  const unsigned wanted = unsigned(std::ceil(span / spacing)) + 1;
  return std::clamp(wanted, 2u, MAX_TERRAIN_LATERAL_SAMPLES);
}

/** Set false to disable terrain slope shading and the sun disc. */
static constexpr bool SLOPE_SHADING_ENABLED = true;

/** Hillshade cast shadows along the sun direction on the terrain mesh. */
static constexpr bool CAST_SHADOWS_ENABLED = true;

/** Sky dome extends this far below the geographic horizon (rad). */
static constexpr double SKY_BELOW_HORIZON = 0.008;

/** Minimum elevation band (rad) for terrain→sky compositing above horizon. */
static constexpr double HORIZON_COMPOSITE_BAND = 0.0025;

/** Terrain at/above eye Z minus this (m) keeps full opacity (ridges/peaks). */
static constexpr double HORIZON_PEAK_CLEARANCE = 20.;

/** Start horizon compositing beyond this fraction of horizon distance. */
static constexpr double HORIZON_COMPOSITE_DIST = 0.06;

/** Hillshade contrast on the muted land base (higher = more relief). */
static constexpr double SHADING_CONTRAST = 165.;

/** Soft ambient lift for north-facing slopes (0 = full contrast). */
static constexpr double SHADING_AMBIENT = 0.18;

/** Cast-shadow darkness target (negative, matches ApplyTerrainShading range). */
static constexpr int CAST_SHADOW_ILLUM = -34;

/** Minimum sun elevation used for hillshade (avoids flat lighting at dusk). */
static constexpr double MIN_SUN_ELEVATION_DEG = 28.;

/** Draped mesh grid on the terrain surface (EFIS-style reference). */
static constexpr bool TERRAIN_GRID_ENABLED = true;

/** Grid visible out to this slant range (m); fades to zero by @ref GRID_FADE_FULL. */
static constexpr double GRID_FADE_START = 3500.;
static constexpr double GRID_FADE_FULL = 9000.;

/** Lift grid lines above the terrain surface to reduce z-fighting (m). */
static constexpr double TERRAIN_GRID_OFFSET = 1.5;

/** Grid line strength in [0, 1] from slant range. */
inline double
GridLineStrength(double slant_range) noexcept
{
  if (slant_range <= GRID_FADE_START)
    return 1.;

  if (slant_range >= GRID_FADE_FULL)
    return 0.;

  const double t = (slant_range - GRID_FADE_START) /
    (GRID_FADE_FULL - GRID_FADE_START);
  return 1. - t * t;
}

/**
 * Geographic lateral (+ = right of track) to OpenGL world Y.
 * lookAt(..., +Z up) maps world +Y to screen-left; negate so right is right.
 */
inline double
DisplayLateral(double geo_lateral) noexcept
{
  return -geo_lateral;
}

inline double
SlantRange(double x, double y, double z, double ref_alt) noexcept
{
  const double dx = x;
  const double dy = y;
  const double dz = z - ref_alt;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

/** Scale terrain relief around @p vertical_ref (not the live aircraft altitude). */
inline double
ExaggeratedElevation(double raw_msl, double vertical_ref, bool special,
                     double surface_offset = 0.) noexcept
{
  if (special)
    return 0.;

  return raw_msl + (raw_msl - vertical_ref) *
         (VERTICAL_EXAGGERATION - 1.) + surface_offset;
}

/** Eye Z in aircraft-relative coordinates; clears local terrain under the aircraft. */
inline double
CameraEyeZ(double local_ground_z) noexcept
{
  return std::max(CAMERA_EYE_MIN, local_ground_z + CAMERA_EYE_CLEARANCE);
}

/** Earth curvature drop below the local tangent plane at (x, y) (m). */
inline double
EarthCurvatureDrop(double x, double y) noexcept
{
  return (x * x + y * y) / (2. * double(FAISphere::REARTH));
}

/** Height in aircraft-relative coordinates (m); negative = below the aircraft. */
inline double
RelativeTerrainZ(double raw_msl, double aircraft_alt, double vertical_ref,
                 double x, double y, bool invalid) noexcept
{
  if (invalid)
    return 0.;

  return (raw_msl - aircraft_alt) +
         (raw_msl - vertical_ref) * (VERTICAL_EXAGGERATION - 1.) -
         EarthCurvatureDrop(x, y);
}

inline double
DisplayElevation(double raw_msl, double vertical_ref, bool special) noexcept
{
  return ExaggeratedElevation(raw_msl, vertical_ref, special,
                              TOPO_SURFACE_OFFSET);
}

inline double
HorizontalFovRadians(float aspect) noexcept
{
  const double vfov = Angle::Degrees(CAMERA_FOV_DEG).Radians();
  return 2. * std::atan(std::tan(vfov * 0.5) * double(aspect));
}

inline double
LateralHalfWidthAtDistance(double dist, float aspect) noexcept
{
  const double effective = std::max(dist, MIN_TERRAIN_DIST);
  return effective * std::tan(HorizontalFovRadians(aspect) * 0.5) * FRUSTUM_MARGIN;
}

/** Eye AMSL when @p z=0 is aircraft altitude @p ref_alt. */
inline double
EyeAltitudeMSL(double ref_alt, double eye_z) noexcept
{
  return ref_alt + eye_z;
}

inline double
HorizonElevationRadians(double height_amsl) noexcept
{
  if (height_amsl <= 1.)
    return 0.;

  return -std::sqrt(2. * height_amsl / double(FAISphere::REARTH));
}

/** Geographic horizon dip for the camera eye (rad, negative = below level). */
inline double
EyeHorizonElevation(double ref_alt, double eye_z) noexcept
{
  return HorizonElevationRadians(EyeAltitudeMSL(ref_alt, eye_z));
}

/** Geometric horizon distance for @p height_amsl (m). */
inline double
HorizonDistance(double height_amsl) noexcept
{
  if (height_amsl <= 1.)
    return 0.;

  return std::sqrt(2. * height_amsl * double(FAISphere::REARTH));
}

/** Elevation angle (rad) from the eye to @p (x,y,z). */
inline double
TerrainElevationAngle(double z, double x, double y,
                      double eye_z) noexcept
{
  return std::atan2(z - eye_z, std::max(std::hypot(x, y), 1.));
}

/** Per-vertex sky/terrain compositing at the shared geographic horizon. */
struct HorizonComposite {
  /** Blend terrain RGB toward horizon colour [0..1]. */
  double color_blend = 0.;
  /** 1 = opaque terrain; 0 = sky shows through (alpha compositing). */
  double alpha = 1.;
};

/**
 * Compositing factors for terrain that projects above the geographic
 * horizon. Uses the same eye horizon as the sky dome (@ref EyeHorizonElevation).
 */
inline HorizonComposite
HorizonCompositeAt(double z, double x, double y, double eye_z,
                   double ref_alt) noexcept
{
  HorizonComposite c;

  if (z >= eye_z - HORIZON_PEAK_CLEARANCE)
    return c;

  const double horizon = EyeHorizonElevation(ref_alt, eye_z);
  const double elev = TerrainElevationAngle(z, x, y, eye_z);
  if (elev <= horizon)
    return c;

  const double d = std::hypot(x, y);
  const double eye_amsl = EyeAltitudeMSL(ref_alt, eye_z);
  const double d_horizon = HorizonDistance(eye_amsl);
  if (d_horizon <= 0. || d < d_horizon * HORIZON_COMPOSITE_DIST)
    return c;

  const double band =
    std::max(HORIZON_COMPOSITE_BAND, -horizon * 0.28);
  const double t = std::clamp((elev - horizon) / band, 0., 1.);

  c.color_blend = t;
  c.alpha = 1. - t;

  return c;
}

} // namespace ForwardViewGeometry
