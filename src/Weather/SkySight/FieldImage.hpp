// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContourRaster.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Point2D.hpp"

#include <cstdint>

class Path;

namespace SkySight {

/**
 * How a forecast field is packed into the 8-bit samples of its overlay
 * file: sample 0 marks missing data, 1..255 spans [minimum, maximum]
 * linearly.
 *
 * Eight bits are plenty here.  A legend covers its range in roughly a
 * dozen bands, so a quantisation step is a small fraction of the narrowest
 * band, and the resulting shift of a contour is far below one pixel.  It
 * also keeps a cached forecast smaller than the pre-classified RGBA image
 * this format replaced.
 */
struct FieldQuantisation {
  float minimum = 0;
  float maximum = 0;

  [[nodiscard]] bool IsValid() const noexcept;

  [[nodiscard]] [[gnu::pure]] uint8_t Encode(float value) const noexcept;

  /** @return the decoded value, or NaN when @p sample marks missing data */
  [[nodiscard]] [[gnu::pure]] float Decode(uint8_t sample) const noexcept;
};

/**
 * Derive the packing range from the samples present in @p field.  Returns
 * an invalid quantisation when the field holds no usable sample.
 */
[[nodiscard]] FieldQuantisation
MeasureField(const ScalarField &field) noexcept;

/** A forecast field together with the geographic grid it samples. */
struct GeoScalarField {
  ScalarField field;

  /** North-west corner of the area covered, not the first sample centre. */
  GeoPoint north_west = GeoPoint::Invalid();

  /** Degrees per sample; latitude runs south as the row index grows. */
  double longitude_step = 0;
  double latitude_step = 0;

  [[nodiscard]] bool IsValid() const noexcept;

  [[nodiscard]] [[gnu::pure]] GeoBounds GetBounds() const noexcept;

  /**
   * The sample column and row covering @p p, clamped to the grid.
   */
  [[nodiscard]] [[gnu::pure]] IntPoint2D
  ProjectClamped(GeoPoint p) const noexcept;

  /** The north-west corner of the area covered by sample (x, y). */
  [[nodiscard]] [[gnu::pure]] GeoPoint
  GetSampleCorner(unsigned x, unsigned y) const noexcept;
};

#ifdef USE_GEOTIFF

/**
 * Write @p field as the single-band GeoTIFF that overlay rendering reads
 * back.  Throws on error.
 */
void
WriteFieldImage(Path path, const GeoScalarField &field);

/**
 * Read a field written by #WriteFieldImage.  Throws on error, including
 * when the file is some other image; that is what keeps a stale overlay
 * from an earlier decoder out of the map.
 */
[[nodiscard]] GeoScalarField
ReadFieldImage(Path path);

#endif

} // namespace SkySight
