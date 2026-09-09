// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ContourRaster.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>
#include <limits>

namespace SkySight {

namespace {

constexpr float MISSING = std::numeric_limits<float>::quiet_NaN();

[[nodiscard]] [[gnu::const]] unsigned
ClampIndex(int index, unsigned size) noexcept
{
  if (index <= 0)
    return 0;

  const auto value = unsigned(index);
  return value < size ? value : size - 1;
}

struct CubicWeights {
  float value[4];
};

/**
 * Catmull-Rom basis for a position @p t within the interval between the
 * second and third of four consecutive samples.
 */
[[nodiscard]] [[gnu::const]] constexpr CubicWeights
GetCubicWeights(float t) noexcept
{
  const float t2 = t * t;
  const float t3 = t2 * t;

  return {{
    -0.5f * t3 + t2 - 0.5f * t,
    1.5f * t3 - 2.5f * t2 + 1.0f,
    -1.5f * t3 + 2.0f * t2 + 0.5f * t,
    0.5f * t3 - 0.5f * t2,
  }};
}

} // namespace

float
SampleField(const ScalarField &field, float x, float y) noexcept
{
  if (field.empty())
    return MISSING;

  const int x_index = int(std::floor(x));
  const int y_index = int(std::floor(y));
  const float fx = x - float(x_index);
  const float fy = y - float(y_index);

  /* the 4x4 neighbourhood, repeating edge samples outside the grid */
  unsigned columns[4], rows[4];
  for (int i = 0; i < 4; ++i) {
    columns[i] = ClampIndex(x_index - 1 + i, field.width);
    rows[i] = ClampIndex(y_index - 1 + i, field.height);
  }

  float neighbourhood[4][4];
  bool complete = true;
  for (int row = 0; row < 4; ++row)
    for (int column = 0; column < 4; ++column) {
      const float value = field.Get(columns[column], rows[row]);
      neighbourhood[row][column] = value;
      complete &= std::isfinite(value);
    }

  /* bilinear over the inner 2x2, skipping missing samples so that data
     edges do not grow a transparent fringe */
  const float weights[2][2] = {
    {(1 - fx) * (1 - fy), fx * (1 - fy)},
    {(1 - fx) * fy, fx * fy},
  };

  float sum = 0, weight_sum = 0;
  float minimum = std::numeric_limits<float>::infinity();
  float maximum = -std::numeric_limits<float>::infinity();
  for (int row = 0; row < 2; ++row)
    for (int column = 0; column < 2; ++column) {
      const float value = neighbourhood[row + 1][column + 1];
      if (!std::isfinite(value))
        continue;

      sum += weights[row][column] * value;
      weight_sum += weights[row][column];
      minimum = std::min(minimum, value);
      maximum = std::max(maximum, value);
    }

  if (weight_sum < 0.5f)
    /* mostly outside the model domain */
    return MISSING;

  const float bilinear = sum / weight_sum;
  if (!complete)
    return bilinear;

  const auto weights_x = GetCubicWeights(fx);
  const auto weights_y = GetCubicWeights(fy);

  float bicubic = 0;
  for (int row = 0; row < 4; ++row) {
    float row_value = 0;
    for (int column = 0; column < 4; ++column)
      row_value += weights_x.value[column] * neighbourhood[row][column];

    bicubic += weights_y.value[row] * row_value;
  }

  /* Catmull-Rom overshoots near steep gradients; clamping to the
     surrounding samples keeps the smoothing from creating bands the data
     never reaches. */
  return std::clamp(bicubic, minimum, maximum);
}

ContourPalette::ContourPalette(const std::map<float, LegendColor> &legend)
{
  bands.reserve(legend.size());

  /* The last non-positive stop covers the near-zero background of bipolar
     overlays such as ridge lift. */
  const auto first_positive = legend.upper_bound(0.f);
  const auto background = first_positive != legend.end() &&
    first_positive != legend.begin()
    ? std::prev(first_positive)
    : legend.end();

  for (auto i = legend.begin(); i != legend.end(); ++i) {
    if (!std::isfinite(i->first))
      continue;

    bands.push_back({i->first, i->second, i != background});
  }
}

const LegendColor *
ContourPalette::Find(float value) const noexcept
{
  if (bands.empty() || !std::isfinite(value))
    return nullptr;

  auto i = std::upper_bound(bands.begin(), bands.end(), value,
                            [](float sample, const Band &band) {
                              return sample < band.threshold;
                            });
  if (i == bands.begin())
    return nullptr;

  --i;
  return i->opaque ? &i->color : nullptr;
}

ContourRasterizer::ContourRasterizer(const ScalarField &_field,
                                     const ContourPalette &_palette,
                                     ContourWindow _window) noexcept
  :field(_field), palette(_palette), window(_window)
{
  if (window.upsample < 1)
    window.upsample = 1;
}

float
ContourRasterizer::SampleAt(unsigned x, unsigned y) const noexcept
{
  const float scale = 1.0f / float(window.upsample);

  /* output pixel centres, expressed in grid samples */
  return SampleField(field,
                     float(window.x) + (float(x) + 0.5f) * scale - 0.5f,
                     float(window.y) + (float(y) + 0.5f) * scale - 0.5f);
}

void
ContourRasterizer::RenderRow(unsigned y, std::span<uint8_t> row) const noexcept
{
  const unsigned width = GetWidth();
  assert(row.size() >= std::size_t(width) * 4);

  auto *pixel = row.data();
  for (unsigned x = 0; x < width; ++x, pixel += 4) {
    const auto *color = palette.Find(SampleAt(x, y));
    if (color == nullptr) {
      pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
      continue;
    }

    pixel[0] = color->red;
    pixel[1] = color->green;
    pixel[2] = color->blue;
    pixel[3] = 255;
  }
}

} // namespace SkySight
