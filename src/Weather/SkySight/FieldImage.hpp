// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContourRaster.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Point2D.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

class Path;

namespace SkySight {

/**
 * Turn one raw value of a SkySight NetCDF grid into the value the
 * legend is looked up with, applying "scale_factor" and "add_offset".
 *
 * @param fill_value the variable's "_FillValue", or nothing if it
 * has none
 * @return nothing for a sample without a value: not finite, equal to
 * the fill value, or outside the float range after scaling
 */
[[nodiscard]] [[gnu::const]] inline std::optional<float>
DecodeGridValue(double raw, std::optional<double> fill_value,
                double scale, double offset) noexcept
{
  if (!std::isfinite(raw) || (fill_value && raw == *fill_value))
    return std::nullopt;

  /* a double outside the float range must not be converted: that is
     undefined behaviour, not infinity */
  const double value = raw * scale + offset;
  if (!std::isfinite(value) ||
      std::fabs(value) > std::numeric_limits<float>::max())
    return std::nullopt;

  return static_cast<float>(value);
}

/**
 * Derive the packing range from the float samples in @p field.  Returns
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
