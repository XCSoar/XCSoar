// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "GeoPoint.hpp"
#include "Quadrilateral.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * A regular lattice sampling the georeference of a raster (e.g. a
 * GeoTIFF overlay). The raster is divided into nx * ny cells; node
 * (i,j) with 0<=i<=nx and 0<=j<=ny holds the WGS84 position at relative
 * raster position (i/nx, j/ny) measured from the top-left corner.
 *
 * A 1x1 grid just contains the 4 corners. Subdividing the raster
 * lets a renderer follow a curved map projection more accurately.
 * The remaining error scales as 1/n^2.
 */
struct GeoReferencedGrid {
  std::uint_least16_t nx = 1, ny = 1;

  /** (nx+1)*(ny+1) nodes, row-major: index = j*(nx+1) + i. */
  std::vector<GeoPoint> points;

  /** Construct a trivial 1x1 grid with default-constructed corners. */
  GeoReferencedGrid() noexcept
    :points((std::size_t(nx) + 1) * (std::size_t(ny) + 1)) {}

  /** Construct a trivial 1x1 grid from a quadrilateral's corners. */
  explicit GeoReferencedGrid(const GeoQuadrilateral &q) noexcept
    :points{q.top_left, q.top_right, q.bottom_left, q.bottom_right} {}

  const GeoPoint &At(unsigned i, unsigned j) const noexcept {
    return points[j * (std::size_t(nx) + 1) + i];
  }

  /** Is the raster subdivided into more than one cell? */
  bool IsMesh() const noexcept {
    return nx > 1 || ny > 1;
  }

  /**
   * The four outer corners, in #GeoQuadrilateral order
   * (top_left, top_right, bottom_left, bottom_right).
   */
  GeoQuadrilateral GetCornerQuadrilateral() const noexcept {
    return {At(0, 0), At(nx, 0), At(0, ny), At(nx, ny)};
  }
};
