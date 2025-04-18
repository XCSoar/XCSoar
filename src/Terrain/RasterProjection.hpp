// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RasterTraits.hpp"
#include "RasterLocation.hpp"
#include "Geo/GeoPoint.hpp"

class GeoBounds;

/**
 * This class manages the projection from GeoPoint to RasterMap
 * coordinates.
 */
class RasterProjection {
  int left, top;
  double x_scale, y_scale;

public:
  /**
   * Initialise the projection.
   *
   * @param bounds the geographic bounds of the map file
   * @param width the sub-pixel ("fine)" width of the map file
   * @param height the sub-pixel ("fine)" height of the map file
   */
  void Set(const GeoBounds &bounds, UnsignedPoint2D size) noexcept;

  constexpr Angle WidthToAngle(double pixels) const noexcept {
    return Angle::Native(pixels / x_scale);
  }

  constexpr Angle HeightToAngle(double pixels) const noexcept {
    return Angle::Native(pixels / y_scale);
  }

  constexpr int AngleToWidth(Angle angle) const noexcept {
    return (int)(angle.Native() * x_scale);
  }

  constexpr int AngleToHeight(Angle angle) const noexcept {
    return (int)(angle.Native() * y_scale);
  }

  constexpr SignedRasterLocation ProjectFine(GeoPoint location) const noexcept {
    return SignedRasterLocation(AngleToWidth(location.longitude) - left,
                                top - AngleToHeight(location.latitude));
  }

  constexpr GeoPoint UnprojectFine(SignedRasterLocation coords) const noexcept {
    return GeoPoint(WidthToAngle((int)coords.x + left),
                    HeightToAngle(top - (int)coords.y));
  }

  constexpr SignedRasterLocation ProjectCoarse(GeoPoint location) const noexcept {
    return ProjectFine(location) >> RasterTraits::SUBPIXEL_BITS;
  }

  constexpr GeoPoint UnprojectCoarse(SignedRasterLocation coords) const noexcept {
    return UnprojectFine(coords << RasterTraits::SUBPIXEL_BITS);
  }

  constexpr SignedRasterLocation ProjectCoarseRound(GeoPoint location) const noexcept {
    return ProjectFine(location).RoundingRightShift(RasterTraits::SUBPIXEL_BITS);
  }

  /**
   * Determines the distance (in meters) of two raster pixels.
   *
   * @param pixels the pixel distance between two pixels
   */
  [[gnu::pure]]
  double FinePixelDistance(const GeoPoint &location,
                           unsigned pixels) const noexcept;

  [[gnu::pure]]
  double CoarsePixelDistance(const GeoPoint &location,
                             unsigned pixels) const noexcept
  {
    /* factor 256 because the caller should pass a physical pixel
       number, not interpolated */
    return FinePixelDistance(location, pixels << RasterTraits::SUBPIXEL_BITS);
  }

  /**
   * Converts a distance (in meters) to a pixel distance.
   *
   * @param pixels the pixel distance between two pixels
   */
  [[gnu::pure]]
  unsigned
  DistancePixelsFine(double distance) const noexcept;

  [[gnu::pure]]
  unsigned DistancePixelsCoarse(double distance) const noexcept {
    return DistancePixelsFine(distance) >> RasterTraits::SUBPIXEL_BITS;
  }
};
