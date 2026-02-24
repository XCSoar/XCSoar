// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Terrain/RasterRenderer.hpp"
#include "Terrain/RasterMap.hpp"
#include "Math/Constants.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Ramp.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/RawBitmap.hpp"
#include "Renderer/GeoBitmapRenderer.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/event/Idle.hpp"

#include <algorithm> // for std::clamp()
#include <cassert>
#include <cstdint>

/**
 * Constants for terrain rendering thresholds and quantisation limits.
 * These values were introduced in commit df6c73466b to replace magic numbers.
 */
static constexpr double PIXEL_SIZE_NORMAL_THRESHOLD = 3000.0;
static constexpr double PIXEL_SIZE_LOW_ZOOM_THRESHOLD = 20000.0;
static constexpr double ZOOM_FACTOR_DIVISOR = 4250.0;
static constexpr unsigned MAX_QUANTISATION_NEAR = 25;
static constexpr unsigned MAX_QUANTISATION_LOW_ZOOM = 40;
static constexpr double BOUNDS_SCALE_FACTOR = 1.5;

/**
 * Interpolate between x and y with i/128, i.e. i/(1 << 7).
 *
 * i must be below or equal to 128.
 */
static constexpr unsigned
MIX(unsigned x, unsigned y, unsigned i) noexcept
{
  return (x * i + y * ((1 << 7) - i)) >> 7;
}

/**
 * Shade the given color according to the illumination value.
 *
 * illum = 64: Contour, mixed with 50% brown
 * illum < 0:  Shadow, mixed with up to 50% dark blue
 * illum > 0:  Highlight, mixed with up to 25% yellow
 * illum = 0:  No shading
 */
static constexpr RawColor
TerrainShading(const int illum, RGB8Color color) noexcept
{
  if (illum == -64) {
    // brown color mixed in for contours
    return RawColor(MIX(100, color.Red(), 64),
                    MIX(70, color.Green(), 64),
                    MIX(26, color.Blue(), 64));
  } else if (illum < 0) {
    // shadow to blue
    int x = std::min(63, -illum);
    return RawColor(MIX(0, color.Red(), x),
                    MIX(0, color.Green(), x),
                    MIX(64, color.Blue(), x));
  } else if (illum > 0) {
    // highlight to yellow
    int x = std::min(32, illum / 2);
    return RawColor(MIX(255, color.Red(), x),
                    MIX(255, color.Green(), x),
                    MIX(16, color.Blue(), x));
  } else
    return RawColor(color.Red(), color.Green(), color.Blue());
}

static constexpr unsigned
ContourInterval(unsigned h, unsigned contour_height_scale) noexcept
{
  return std::min(254u, h >> contour_height_scale);
}

[[gnu::const]]
static unsigned
ContourInterval(const TerrainHeight h, const unsigned contour_height_scale)
{
  if (h.IsSpecial()) [[unlikely]]
    return 0;

  if (h.GetValue() <= 0)
    return 0;

  return ContourInterval(h.GetValue(), contour_height_scale);
}

RasterRenderer::RasterRenderer() noexcept = default;

RasterRenderer::~RasterRenderer() noexcept
{
  delete[] color_table;
  delete image;
  delete[] contour_column_base;
}

#ifdef ENABLE_OPENGL

[[gnu::pure]]
static unsigned
GetQuantisation() noexcept
{
  if (IsUserIdle(1500))
    /* full terrain resolution when the user stops interacting */
    return 1;
  else if (IsUserIdle(750))
    /* reduced terrain resolution when the user has interacted with
       XCSoar recently */
    return 2;
  else
    /* the user is actively operating XCSoar: reduce UI latency */
    return Layout::FastScale(2);
}

bool
RasterRenderer::UpdateQuantisation() noexcept
{
  if (fixed_quantisation)
    /* value was set explicitly via SetQuantisationPixels();
       don't let the idle-based heuristic overwrite it */
    return quantisation_pixels < last_quantisation_pixels;

  quantisation_pixels = GetQuantisation();
  return quantisation_pixels < last_quantisation_pixels;
}

const GLTexture &
RasterRenderer::BindAndGetTexture() const noexcept
{
  return image->BindAndGetTexture();
}

#endif

void
RasterRenderer::ScanMap(const RasterMap &map,
                        const WindowProjection &projection) noexcept
{
  // Coordinates of the MapWindow center
  const auto p = projection.GetScreenCenter();
  // GeoPoint corresponding to the MapWindow center
  GeoPoint center = projection.ScreenToGeo(p);
  // GeoPoint "next to" Gmid (depends on terrain resolution)
  GeoPoint neighbor = projection.ScreenToGeo(p + PixelSize{quantisation_pixels});

  // Geographical edge length of pixel in the MapWindow center in meters
  pixel_size = M_SQRT1_2 * center.DistanceS(neighbor);

  // set resolution

  if (pixel_size < PIXEL_SIZE_NORMAL_THRESHOLD) {
    // Data point size of the (terrain) map in meters multiplied by 256
    auto map_pixel_size = map.PixelDistance(center, 1);

    // How many screen pixels does one data point stretch?
    auto q = map_pixel_size / pixel_size;

    /* round down to reduce slope shading artefacts (caused by
       RasterBuffer interpolation) */
    quantisation_effective = std::max(1, (int)q);

    /* when zoomed in very near, use a large fixed area for slope
       calculation to ensure terrain shading still works */
    const unsigned max_quantisation_near = Layout::FastScale(MAX_QUANTISATION_NEAR);
    if (quantisation_effective > max_quantisation_near)
      quantisation_effective = max_quantisation_near;

  } else if (pixel_size < PIXEL_SIZE_LOW_ZOOM_THRESHOLD) {
    auto map_pixel_size = map.PixelDistance(center, 1);
    auto q = map_pixel_size / pixel_size;

    /* At low zoom levels (3000-20000m pixel size), use adaptive coarse
       quantisation instead of completely disabling shading. Scale
       quantisation based on pixel_size: at 3000m use calculated q
       (transition from normal mode), at 20000m use ~4x coarser
       quantisation for better performance */
    const double zoom_factor = 1.0 + (pixel_size - PIXEL_SIZE_NORMAL_THRESHOLD) / ZOOM_FACTOR_DIVISOR;
    quantisation_effective = std::max(1, (int)(q * zoom_factor));

    /* Cap at reasonable maximum to avoid artifacts and maintain
       performance. Higher cap than normal mode since we're at low zoom */
    const unsigned max_quantisation_low_zoom = Layout::FastScale(MAX_QUANTISATION_LOW_ZOOM);
    if (quantisation_effective > max_quantisation_low_zoom)
      quantisation_effective = max_quantisation_low_zoom;

  } else {
    /* disable slope shading when zoomed out extremely far (pixel_size >= 20000m)
       as terrain features become too small to be meaningful and performance
       would suffer with reasonable quantisation */
    quantisation_effective = 0;
  }

#ifdef ENABLE_OPENGL
  bounds = projection.GetScreenBounds().Scale(BOUNDS_SCALE_FACTOR);
  bounds.IntersectWith(map.GetBounds());

  height_matrix.Fill(map, bounds,
                     (UnsignedPoint2D)projection.GetScreenSize() / quantisation_pixels,
                     true);

  last_quantisation_pixels = quantisation_pixels;
#else
  height_matrix.Fill(map, projection, quantisation_pixels, true);
#endif
}

void
RasterRenderer::GenerateImage(bool do_shading,
                              unsigned height_scale,
                              int contrast, int brightness,
                              const Angle sunazimuth,
                              bool do_contour) noexcept
{
  if (image == nullptr ||
      height_matrix.GetSize().x > image->GetSize().width ||
      height_matrix.GetSize().y > image->GetSize().height) {
    delete image;
    image = new RawBitmap(PixelSize{height_matrix.GetSize()});

    delete[] contour_column_base;
    contour_column_base = new unsigned char[height_matrix.GetSize().x];
  }

  if (quantisation_effective == 0) {
    do_shading = false;
    do_contour = false;
  }

  const unsigned contour_height_scale = do_contour? height_scale * 2 : 16;

  ContourStart(contour_height_scale);

  if (do_shading)
    GenerateSlopeImage(height_scale, contrast, brightness,
                       sunazimuth, contour_height_scale);
  else
    GenerateUnshadedImage(height_scale, contour_height_scale);

  image->SetDirty();
}

void
RasterRenderer::GenerateUnshadedImage(const unsigned height_scale,
                                      const unsigned contour_height_scale) noexcept
{
  const auto *src = height_matrix.GetData();
  const RawColor *oColorBuf = color_table + 64 * 256;
  RawColor *dest = image->GetTopRow();

  for (unsigned y = height_matrix.GetSize().y; y > 0; --y) {
    RawColor *p = dest;
    dest = image->GetNextRow(dest);

    unsigned contour_row_base = ContourInterval(*src, contour_height_scale);
    unsigned char *contour_this_column_base = contour_column_base;

    for (unsigned x = height_matrix.GetSize().x; x > 0; --x) {
      const auto e = *src++;
      if (!e.IsSpecial()) [[likely]] {
        unsigned h = std::max(0, (int)e.GetValue());

        const unsigned contour_interval =
          ContourInterval(h, contour_height_scale);

        h = std::min(254u, h >> height_scale);
        if (contour_interval != contour_row_base ||
            contour_interval != *contour_this_column_base) [[unlikely]] {
          *p++ = oColorBuf[(int)h - 64 * 256];
          *contour_this_column_base = contour_row_base = contour_interval;
        } else {
          *p++ = oColorBuf[h];
        }
      } else if (e.IsWater()) {
        // we're in the water, so look up the color for water
        *p++ = oColorBuf[255];
      } else {
        /* outside the terrain file bounds: white background */
        *p++ = RawColor(0xff, 0xff, 0xff);
      }
      contour_this_column_base++;

    }
  }
}

/**
 * Clip the difference between two adjacent terrain height values to
 * sane bounds.  This works around integer overflows in the
 * GenerateSlopeImage() formula when the map file is broken, avoiding
 * the sqrt() call with a negative argument.
 */
static constexpr int
ClipHeightDelta(int d) noexcept
{
  return std::clamp(d, -512, 512);
}

static constexpr int
ClipHeightDelta(TerrainHeight a, TerrainHeight b) noexcept
{
  return ClipHeightDelta(a.GetValue() - b.GetValue());
}

// JMW: if zoomed right in (e.g. one unit is larger than terrain
// grid), then increase the step size to be equal to the terrain
// grid for purposes of calculating slope, to avoid shading problems
// (gridding of display) This is why quantisation_effective is used instead of 1
// previously.  for large zoom levels, quantisation_effective=1
void
RasterRenderer::GenerateSlopeImage(unsigned height_scale,
                                   int contrast,
                                   const int sx, const int sy, const int sz,
                                   const unsigned contour_height_scale) noexcept
{
  assert(quantisation_effective > 0);

  const auto border = PixelRect{PixelSize{height_matrix.GetSize()}}
    .WithPadding(quantisation_effective);

  const unsigned height_slope_factor =
    std::clamp((unsigned)pixel_size, 1u,
               /* this upper limit avoids integer overflows in the
                  "mag" formula; it effectively limits "dd2" so
                  calculating its square will not overflow */
               8192u / (quantisation_effective * quantisation_effective));
  
  const auto *src = height_matrix.GetData();
  const RawColor *oColorBuf = color_table + 64 * 256;

  RawColor *dest = image->GetTopRow();

  for (unsigned y = 0; y < height_matrix.GetSize().y; ++y) {
    const unsigned row_plus_index = y < (unsigned)border.bottom
      ? quantisation_effective
      : height_matrix.GetSize().y - 1 - y;
    const unsigned row_plus_offset = height_matrix.GetSize().x * row_plus_index;

    const unsigned row_minus_index = y >= quantisation_effective
      ? quantisation_effective : y;
    const unsigned row_minus_offset = height_matrix.GetSize().x * row_minus_index;

    const unsigned p31 = row_plus_index + row_minus_index;

    RawColor *p = dest;
    dest = image->GetNextRow(dest);

    unsigned contour_row_base = ContourInterval(*src, contour_height_scale);
    unsigned char *contour_this_column_base = contour_column_base;

    for (unsigned x = 0; x < height_matrix.GetSize().x; ++x, ++src) {
      const auto e = *src;
      if (!e.IsSpecial()) [[likely]] {
        unsigned h = std::max(0, (int)e.GetValue());

        const unsigned contour_interval =
          ContourInterval(h, contour_height_scale);

        h = std::min(254u, h >> height_scale);

        // no need to calculate slope if undefined height or sea level

        // Y direction
        assert(src - row_minus_offset >= height_matrix.GetData());
        assert(src + row_plus_offset >= height_matrix.GetData());
        assert(src - row_minus_offset < height_matrix.GetDataEnd());
        assert(src + row_plus_offset < height_matrix.GetDataEnd());

        // X direction

        const unsigned column_plus_index = x < (unsigned)border.right
          ? quantisation_effective
          : height_matrix.GetSize().x - 1 - x;
        const unsigned column_minus_index = x >= (unsigned)border.left
          ? quantisation_effective : x;

        assert(src - column_minus_index >= height_matrix.GetData());
        assert(src + column_plus_index >= height_matrix.GetData());
        assert(src - column_minus_index < height_matrix.GetDataEnd());
        assert(src + column_plus_index < height_matrix.GetDataEnd());

        const auto h_above = src[-(int)row_minus_offset];
        const auto h_below = src[row_plus_offset];
        const auto h_left = src[-(int)column_minus_index];
        const auto h_right = src[column_plus_index];

        if (h_above.IsSpecial() || h_below.IsSpecial() ||
            h_left.IsSpecial() || h_right.IsSpecial()) [[unlikely]] {
          /* some "special" terrain value surrounding us (water or
             invalid), skip slope calculation */
          *p++ = oColorBuf[h];
          contour_this_column_base++;
          continue;
        }

        if (contour_interval != contour_row_base ||
            contour_interval != *contour_this_column_base) [[unlikely]] {

          *contour_this_column_base++ = contour_row_base = contour_interval;
          *p++ = oColorBuf[int(h) - 64 * 256];
          continue;
        }

        const int p32 = ClipHeightDelta(h_above, h_below);
        const int p22 = ClipHeightDelta(h_right, h_left);

        const unsigned p20 = column_plus_index + column_minus_index;

        const int dd0 = p22 * int(p31);
        const int dd1 = int(p20) * p32;
        const double dd2 = double(p20) * double(p31) *
          double(height_slope_factor);
        const double num =
          dd2 * double(sz) + double(dd0) * double(sx) +
          double(dd1) * double(sy);
        const double square_mag =
          double(dd0) * double(dd0) +
          double(dd1) * double(dd1) +
          dd2 * dd2;
        const double mag = sqrt(square_mag);
        /* this is a workaround for a SIGFPE (division by zero)
           observed by our users on some Android devices (e.g. Nexus
           7), even though we did our best to make sure that the
           integer arithmetics above can't overflow */
        /* TODO: debug this problem and replace this workaround */
        const int sval = int(num / std::max(mag, 1.0));
        const int sindex = (sval - sz) * contrast / 128;
        *p++ = oColorBuf[int(h) + 256 * std::clamp(sindex, -63, 63)];
      } else if (e.IsWater()) {
        // we're in the water, so look up the color for water
        *p++ = oColorBuf[255];
      } else {
        /* outside the terrain file bounds: white background */
        *p++ = RawColor(0xff, 0xff, 0xff);
      }
      contour_this_column_base++;

    }
  }
}

void
RasterRenderer::GenerateSlopeImage(unsigned height_scale,
                                   int contrast, int brightness,
                                   const Angle sunazimuth,
                                   const unsigned contour_height_scale) noexcept
{
  const Angle fudgeelevation = Angle::Degrees(10) +
    Angle::Degrees(80.0 / 255.0) * brightness;

  const int sx = (int)(255 * fudgeelevation.fastcosine() * -sunazimuth.fastsine());
  const int sy = (int)(255 * fudgeelevation.fastcosine() * -sunazimuth.fastcosine());
  const int sz = (int)(255 * fudgeelevation.fastsine());

  GenerateSlopeImage(height_scale, contrast,
                     sx, sy, sz, contour_height_scale);
}

void
RasterRenderer::PrepareColorTable(const ColorRamp *color_ramp, bool do_water,
                                  unsigned height_scale, int interp_levels) noexcept
{
  if (color_table == nullptr)
    color_table = new RawColor[256 * 128];

  for (int i = 0; i < 256; i++) {
    for (int mag = -64; mag < 64; mag++) {
      RawColor color;

      if (i == 255) {
        if (do_water) {
          // water colours
          color = RawColor(85, 160, 255);
        } else {
          color = RawColor(255, 255, 255);

          // ColorRampLookup(0, r, g, b,
          // Color_ramp, NUM_COLOR_RAMP_LEVELS, interp_levels);
        }
      } else {
        const RGB8Color color2 =
          ColorRampLookup(i << height_scale, color_ramp,
                          NUM_COLOR_RAMP_LEVELS, interp_levels);

        color = TerrainShading(mag, color2);
      }

      color_table[i + (mag + 64) * 256] = color;
    }
  }
}

void
RasterRenderer::ContourStart(const unsigned contour_height_scale) noexcept
{
  // initialise column to first row
  const auto *src = height_matrix.GetData();
  unsigned char *col_base = contour_column_base;
  for (unsigned x = height_matrix.GetSize().x; x > 0; --x)
    *col_base++ = ContourInterval(*src++, contour_height_scale);
}

void
RasterRenderer::Draw([[maybe_unused]] Canvas &canvas,
                     const WindowProjection &projection,
                     [[maybe_unused]] bool transparent_white) const noexcept
{
#ifdef ENABLE_OPENGL
  if (bounds.IsValid() && bounds.Overlaps(projection.GetScreenBounds()))
    DrawGeoBitmap(*image,
                  PixelSize{height_matrix.GetSize()},
                  bounds,
                  projection);
#else
  image->StretchTo(PixelSize{height_matrix.GetSize()},
                   canvas, projection.GetScreenSize(),
                   transparent_white);
#endif
}
