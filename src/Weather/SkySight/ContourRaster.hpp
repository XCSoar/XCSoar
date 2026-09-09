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
 * A rectangular grid of scalar forecast samples in display orientation:
 * row 0 is the northern edge, column 0 the western one.  Non-finite
 * values mark missing samples (outside the model domain, fill values).
 */
struct ScalarField {
  std::vector<float> values;
  unsigned width = 0;
  unsigned height = 0;

  [[nodiscard]] bool empty() const noexcept {
    return width == 0 || height == 0 || values.size() != Size();
  }

  [[nodiscard]] constexpr std::size_t Size() const noexcept {
    return std::size_t(width) * std::size_t(height);
  }

  [[nodiscard]] float Get(unsigned x, unsigned y) const noexcept {
    return values[std::size_t(y) * width + x];
  }
};

/**
 * Sample @p field at continuous grid coordinates, where (0, 0) is the
 * centre of the north-west sample.  Coordinates outside the grid clamp to
 * the edge samples.
 *
 * Interpolation is bicubic (Catmull-Rom) and clamped to the range of the
 * four surrounding samples, so smooth contours never invent values that
 * cross a legend threshold the data does not reach.  Neighbourhoods that
 * touch missing samples fall back to bilinear interpolation over the valid
 * ones; the result is missing (NaN) once less than half of the bilinear
 * weight is backed by data.
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

/** The largest upsampling factor a window is built with. */
inline constexpr unsigned MAX_CONTOUR_UPSAMPLE = 8;

/** The patch of a #ScalarField that a contour raster covers. */
struct ContourWindow {
  /** First sample column and row. */
  unsigned x = 0, y = 0;

  /** Sample counts, so the patch spans [x, x + width) x [y, y + height). */
  unsigned width = 0, height = 0;

  /** Size of the raster the patch is rendered into. */
  unsigned raster_width = 0, raster_height = 0;

  [[nodiscard]] constexpr bool empty() const noexcept {
    return width == 0 || height == 0 ||
      raster_width == 0 || raster_height == 0;
  }
};

/**
 * Size the raster for a patch of @p window.width x @p window.height
 * samples, starting from @p upsample output pixels per sample and axis.
 *
 * The result always fits @p max_axis on both axes and @p max_cells in
 * total, which the caller must derive from what the device can actually
 * hold: a whole zoomed-out region is far larger than a magnified patch,
 * and some drivers cap textures at 2048.
 *
 * Shrinking is uniform, so the patch keeps its full extent and only loses
 * detail.  Cropping instead would leave part of the visible map bare.
 */
[[nodiscard]] ContourWindow
FitContourRaster(ContourWindow window, unsigned upsample, unsigned max_axis,
                 std::size_t max_cells) noexcept;

/**
 * Renders part of a forecast grid as filled contour bands.
 *
 * The grid is resampled onto a finer raster and each output pixel is
 * classified by the legend, so a band boundary follows the iso-line of the
 * interpolated field instead of the edge of a grid cell.  Writing one flat
 * pixel per sample instead leaves the map renderer blending *colours*
 * across a whole cell, which smears neighbouring bands into shades the
 * legend does not contain.
 *
 * Rendering only the visible window is what makes this affordable:
 * forecast regions are continent-sized, so upsampling a whole grid would
 * need a texture far beyond what a device can hold, while the patch on
 * screen stays small at any zoom.
 *
 * Holds references; @p field and @p palette must outlive the rasterizer.
 */
class ContourRasterizer {
  const ScalarField &field;
  const ContourPalette &palette;
  ContourWindow window;

  /** Samples per output pixel, below one when the patch is magnified. */
  float x_step, y_step;

public:
  ContourRasterizer(const ScalarField &_field, const ContourPalette &_palette,
                    ContourWindow _window) noexcept;

  [[nodiscard]] unsigned GetWidth() const noexcept {
    return window.raster_width;
  }

  [[nodiscard]] unsigned GetHeight() const noexcept {
    return window.raster_height;
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
