// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Math/Angle.hpp"
#include "ResourceId.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/dim/Size.hpp"

#include <vector>

class RasterMap;
class TopographyStore;

namespace ForwardViewTopography {

struct Vertex {
  float x, y, z;
  float r, g, b, a;
};

/** Screen-aligned sprite at a draped terrain position. */
struct Sprite {
  float x, y, z;
  ResourceId icon, big_icon, ultra_icon;
};

/** Map scale (m/px) for the forward-view corridor. */
double ForwardViewMapScale(const PixelRect &rc, double range,
                           float aspect) noexcept;

WindowProjection CorridorProjection(GeoPoint start, Angle track,
                                    double range, float aspect,
                                    const PixelRect &rc) noexcept;

void BuildSprites(std::vector<Sprite> &sprites,
                  TopographyStore &store,
                  const RasterMap &map,
                  GeoPoint start, GeoVector forward,
                  double aircraft_alt, double vertical_ref_alt,
                  const PixelRect &rc) noexcept;

void BuildOverlay(std::vector<Vertex> &line_vertices,
                  std::vector<Vertex> &fill_vertices,
                  std::vector<Sprite> &sprites,
                  TopographyStore &store,
                  const RasterMap &map,
                  GeoPoint start, GeoVector forward,
                  double aircraft_alt, double vertical_ref_alt,
                  const PixelRect &rc) noexcept;

/** Flat sea-level water polygons for 3D draping (texture path). */
void BuildWaterFills(std::vector<Vertex> &fill_vertices,
                     TopographyStore &store,
                     const RasterMap &map,
                     GeoPoint start, GeoVector forward,
                     double aircraft_alt, double vertical_ref_alt,
                     const PixelRect &rc) noexcept;

} // namespace ForwardViewTopography