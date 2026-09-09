// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Layers.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <vector>

namespace SkySight {

/**
 * 8-bit packing of a forecast field.  Sample 0 is missing data; 1..255
 * spans [minimum, maximum] linearly.
 *
 * A legend covers its range in roughly a dozen bands, so one step is a
 * small fraction of the narrowest band and the contour shift stays below
 * one pixel.  The packed form is also what the overlay keeps in memory.
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
 * Forecast samples, row 0 north and column 0 west.
 *
 * A field built in memory stores floats in #values (missing samples are
 * NaN).  A field loaded from its overlay file stores one byte per sample
 * in #samples and the #packing range, so a continent-sized grid stays
 * about four times smaller than the floats.  The two are not combined.
 */
struct ScalarField {
  std::vector<float> values;
  std::vector<uint8_t> samples;
  FieldQuantisation packing;

  unsigned width = 0;
  unsigned height = 0;

  [[nodiscard]] bool empty() const noexcept {
    if (width == 0 || height == 0)
      return true;

    const auto size = Size();
    if (!samples.empty())
      return samples.size() != size;

    return values.size() != size;
  }

  [[nodiscard]] constexpr std::size_t Size() const noexcept {
    return std::size_t(width) * std::size_t(height);
  }

  [[nodiscard]] float Get(unsigned x, unsigned y) const noexcept {
    const auto index = std::size_t(y) * width + x;
    if (!samples.empty())
      return packing.Decode(samples[index]);

    return values[index];
  }
};

/**
 * Sample @p field at continuous grid coordinates.  (0, 0) is the centre
 * of the north-west sample; coordinates outside the grid clamp to the edge.
 *
 * Bicubic (Catmull-Rom), clamped to the four surrounding samples so the
 * smoothing cannot cross a legend threshold the data does not reach.
 * A neighbourhood that touches missing data falls back to bilinear over
 * the valid samples, and is missing once less than half the weight is
 * backed by data.
 */
[[nodiscard]] [[gnu::pure]] float
SampleField(const ScalarField &field, float x, float y) noexcept;

/**
 * A SkySight legend flattened into a sorted band table, so classifying a
 * sample does not cost a std::map lookup.
 *
 * Colouring is by floor: each stop covers up to the next, and values below
 * the first stop stay transparent.  When the legend has both non-positive
 * and positive stops, the last non-positive band is left transparent too:
 * that band is the near-zero background of bipolar overlays such as ridge
 * lift or convergence, and painting it opaque washes out the map.
 */
class ContourPalette {
  struct Band {
    float threshold;
    LegendColor color;
    /** False for the near-zero background band of bipolar overlays. */
    bool opaque;
  };

  std::vector<Band> bands;

public:
  ContourPalette() noexcept = default;

  /**
   * Non-finite legend thresholds are skipped; they cannot be ordered
   * against samples.
   */
  explicit ContourPalette(const std::map<float, LegendColor> &legend);

  [[nodiscard]] bool empty() const noexcept {
    return bands.empty();
  }

  /**
   * @return the band color for @p value, or nullptr when the sample is
   *         missing, below the first stop, or in the near-zero background
   */
  [[nodiscard]] [[gnu::pure]] const LegendColor *
  Find(float value) const noexcept;
};

/** The largest upsampling factor #ChooseContourUpsample may return. */
inline constexpr unsigned MAX_CONTOUR_UPSAMPLE = 8;

/**
 * Pick the integer factor by which a @p width x @p height patch of a
 * forecast grid is upsampled into a contour raster.
 *
 * Larger factors place the band boundaries more precisely; the caller's
 * @p max_axis and @p max_cells budget bounds the resulting image.
 */
[[nodiscard]] constexpr unsigned
ChooseContourUpsample(unsigned width, unsigned height, unsigned max_axis,
                      std::size_t max_cells) noexcept
{
  if (width == 0 || height == 0)
    return 1;

  for (unsigned factor = MAX_CONTOUR_UPSAMPLE; factor > 1; --factor) {
    const std::size_t scaled_width = std::size_t(width) * factor;
    const std::size_t scaled_height = std::size_t(height) * factor;
    if (scaled_width <= max_axis && scaled_height <= max_axis &&
        scaled_width <= max_cells / scaled_height)
      return factor;
  }

  return 1;
}

/** The patch of a #ScalarField that a contour raster covers. */
struct ContourWindow {
  /** First sample column and row. */
  unsigned x = 0, y = 0;

  /** Sample counts, so the raster spans [x, x + width) x [y, y + height). */
  unsigned width = 0, height = 0;

  /** Output pixels per sample and axis, see #ChooseContourUpsample. */
  unsigned upsample = 1;
};

/**
 * Filled contour bands for one patch of a forecast grid.
 *
 * Each output pixel is classified from the interpolated field, so a band
 * edge follows an iso-line.  One flat colour per sample would let the map
 * blend neighbouring legend colours into shades the legend does not contain.
 *
 * Holds references; @p field and @p palette must outlive the rasterizer.
 */
class ContourRasterizer {
  const ScalarField &field;
  const ContourPalette &palette;
  ContourWindow window;

public:
  ContourRasterizer(const ScalarField &_field, const ContourPalette &_palette,
                    ContourWindow _window) noexcept;

  [[nodiscard]] unsigned GetWidth() const noexcept {
    return window.width * window.upsample;
  }

  [[nodiscard]] unsigned GetHeight() const noexcept {
    return window.height * window.upsample;
  }

  /** The interpolated value at the centre of an output pixel. */
  [[nodiscard]] [[gnu::pure]] float
  SampleAt(unsigned x, unsigned y) const noexcept;

  /**
   * Render output row @p y as premultiplied RGBA, four bytes per pixel.
   * Missing samples and transparent bands are written as zero.
   *
   * @param row a buffer of at least 4 * GetWidth() bytes
   */
  void RenderRow(unsigned y, std::span<uint8_t> row) const noexcept;
};

} // namespace SkySight
