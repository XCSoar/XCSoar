// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#if defined(ENABLE_OPENGL) && defined(HAVE_HTTP)

#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "Topography/ForwardViewTopographyOverlay.hpp"
#include "ui/dim/Rect.hpp"

#include <string>
#include <vector>

class RasterMap;

namespace ForwardViewXCTherm {

struct VolumeCache {
  bool valid = false;
  GeoPoint anchor = GeoPoint::Invalid();
  Angle track = Angle::Zero();
  double range = 0.;
  float aspect = 0.f;
  double ref_alt = 0.;
  std::string parameter;
  unsigned forecast_utc = 0;
  unsigned forecast_coords = 0;
  std::vector<ForwardViewTopography::Vertex> vertices;
};

/**
 * Build semi-transparent extruded XCTherm wind-band polygons for the
 * forward-view corridor. Reuses @p cache when the view and forecast are
 * unchanged.
 */
void BuildVolumes(std::vector<ForwardViewTopography::Vertex> &vertices,
                  VolumeCache &cache,
                  GeoPoint origin, Angle track, double range, float aspect,
                  double ref_alt, double vertical_ref_alt,
                  const RasterMap *terrain_map,
                  const PixelRect &rc) noexcept;

} // namespace ForwardViewXCTherm

#endif
