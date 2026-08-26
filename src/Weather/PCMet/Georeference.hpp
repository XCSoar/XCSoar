// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Point2D.hpp"
#include "ui/dim/Size.hpp"

struct GeoPoint;
class Angle;

namespace PCMet {

/**
 * Maps geographic coordinates to pixels of a pc_met satellite image.
 *
 * The images carry no georeferencing metadata, but each product always
 * shows the same fixed map section, so the parameters can be
 * hard-coded here.  They use the polar stereographic projection of the
 * Deutscher Wetterdienst, the same grid the RADOLAN radar composites
 * are published on: standard parallel 60° North, central meridian
 * 10° East, spherical earth.  The images are axis-aligned to the
 * projection, i.e. there is no rotation, and the pixels are square.
 *
 * Note that the DWD documents this grid for the radar composites
 * only; that the satellite images use it as well was determined by
 * measuring them.
 *
 * The values were derived from the graticule drawn into the
 * "Mitteleuropa" and "Europa" images; see #Georeference.cpp.
 */
struct ImageGeoreference {
  /**
   * The image size the values below refer to.  Images of a different
   * size are assumed to show the same map section and are scaled.
   */
  PixelSize nominal_size;

  /** the width of one pixel in the projection plane [km] */
  double resolution;

  /**
   * The pixel position of the projection origin, i.e. where the North
   * Pole would be.  This is far outside the image.
   */
  DoublePoint2D pole;

  constexpr bool IsDefined() const noexcept {
    return resolution > 0;
  }

  /**
   * Project a geographic location to #nominal_size pixel coordinates,
   * (0,0) being the top left corner.  The result may be outside the
   * image; use IsInside() to check.
   */
  [[gnu::pure]]
  DoublePoint2D ToPixel(const GeoPoint &p) const noexcept;

  /**
   * Is this pixel position inside the image?
   */
  [[gnu::pure]]
  bool IsInside(DoublePoint2D pixel) const noexcept;

  /**
   * The true bearing that corresponds to the "up" direction of the
   * image at this location.  The meridians converge towards the pole,
   * so this is zero only on the central meridian; over Germany it
   * reaches about 5°.  Subtract it from a true bearing to get the
   * angle to draw on screen.
   */
  [[gnu::pure]]
  Angle GetUpBearing(const GeoPoint &p) const noexcept;
};

/**
 * Look up the georeference for a pc_met image.
 *
 * @param type_uri the ImageType::uri of the product
 * @param area_name the ImageArea::name of the map section
 * @return nullptr if the extent of this image is not known
 */
[[gnu::pure]]
const ImageGeoreference *FindImageGeoreference(const char *type_uri,
                                               const char *area_name) noexcept;

} // namespace PCMet
