// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ENABLE_OPENGL

#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "Topography/ForwardViewTopographyOverlay.hpp"
#include "util/Serial.hpp"
#include "ui/dim/Rect.hpp"

#include <vector>

class Airspaces;
struct AirspaceLook;
struct AirspaceRendererSettings;
struct AirspaceComputerSettings;
struct AircraftState;

namespace ForwardViewAirspace {

struct VolumeCache {
  bool valid = false;
  GeoPoint anchor = GeoPoint::Invalid();
  Angle track = Angle::Zero();
  double range = 0.;
  float aspect = 0.f;
  double ref_alt = 0.;
  Serial airspace_serial;
  bool sun_active = false;
  float sun_dir_x = 0.f;
  float sun_dir_y = 0.f;
  float sun_dir_z = 1.f;
  std::vector<ForwardViewTopography::Vertex> fill_vertices;
  std::vector<ForwardViewTopography::Vertex> outline_vertices;
};

void BuildVolumes(std::vector<ForwardViewTopography::Vertex> &fill_vertices,
                  std::vector<ForwardViewTopography::Vertex> &outline_vertices,
                  VolumeCache &cache,
                  const Airspaces &database,
                  const AirspaceLook &look,
                  const AirspaceRendererSettings &renderer_settings,
                  const AirspaceComputerSettings &computer_settings,
                  const AircraftState &aircraft,
                  GeoPoint origin, Angle track, double range, float aspect,
                  double ref_alt, double vertical_ref_alt,
                  double sun_dir_x, double sun_dir_y, double sun_dir_z,
                  bool sun_active,
                  const PixelRect &rc) noexcept;

} // namespace ForwardViewAirspace

#endif
