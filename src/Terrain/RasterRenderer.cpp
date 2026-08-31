// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Terrain/RasterRenderer.hpp"
#include "Terrain/RasterMap.hpp"
#include "Terrain/RasterTileCache.hpp"
#include "Terrain/RasterTile.hpp"
#include "Terrain/RasterTraits.hpp"
#include "Terrain/Height.hpp"
#include "Math/Angle.hpp"
#include "Math/Constants.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Ramp.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/RawBitmap.hpp"
#include "Renderer/GeoBitmapRenderer.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/event/Idle.hpp"
#include "Hardware/CPU.hpp"
#include "LogFile.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/PeriodClock.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/canvas/opengl/ConstantAlpha.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#include "ui/canvas/opengl/Texture.hpp"
#include "ui/canvas/opengl/Shaders.hpp"
#include "ui/canvas/opengl/Program.hpp"
#include "ui/canvas/opengl/Attribute.hpp"
#include "ui/canvas/opengl/VertexPointer.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "ui/dim/Point.hpp"
#endif

#include <algorithm> // for std::clamp()
#ifdef ENABLE_OPENGL
#include <bit>
#endif
#include <cassert>
#include <chrono>
#include <cmath>
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

static void
ApplySlopeQuantisation(unsigned &quantisation_effective,
                       double pixel_size,
                       double map_cell_meters) noexcept
{
  if (pixel_size < PIXEL_SIZE_NORMAL_THRESHOLD) {
    /* How many matrix cells one DEM sample spans.  Round down to
       reduce slope artefacts from RasterBuffer interpolation. */
    auto q = map_cell_meters / pixel_size;
    quantisation_effective = std::max(1, (int)q);
    const unsigned cap = Layout::FastScale(MAX_QUANTISATION_NEAR);
    if (quantisation_effective > cap)
      quantisation_effective = cap;
  } else if (pixel_size < PIXEL_SIZE_LOW_ZOOM_THRESHOLD) {
    auto q = map_cell_meters / pixel_size;
    const double zoom_factor = 1.0 +
      (pixel_size - PIXEL_SIZE_NORMAL_THRESHOLD) / ZOOM_FACTOR_DIVISOR;
    quantisation_effective = std::max(1, (int)(q * zoom_factor));
    const unsigned cap = Layout::FastScale(MAX_QUANTISATION_LOW_ZOOM);
    if (quantisation_effective > cap)
      quantisation_effective = cap;
  } else {
    /* Extremely far: terrain features are too small to shade. */
    quantisation_effective = 0;
  }
}

#ifdef ENABLE_OPENGL
static double
TerrainBoundsScale() noexcept
{
  /* Weak GPUs freeze ScanMap while dragging; a wider overscan keeps
     the last height image covering the view so the white map
     background does not show through. */
  return OpenGL::idle_terrain_quantisation ? 2.5 : BOUNDS_SCALE_FACTOR;
}

static constexpr auto GPU_DEM_STATS_PERIOD = std::chrono::seconds(2);

struct GpuDemStats {
  PeriodClock log_clock;
  unsigned frames = 0;
  unsigned reuse = 0, prep = 0, scan = 0;
  uint64_t prep_us = 0, prep_max_us = 0;
  uint64_t scan_us = 0, scan_max_us = 0;
  uint64_t draw_us = 0, draw_max_us = 0;
  uint64_t gpu_sync_us = 0;
  unsigned quads_ov = 0, quads_tile = 0;
  unsigned uploads = 0;
  uint64_t upload_bytes = 0;
  unsigned last_q = 0, last_qe = 0, last_active = 0;
  unsigned last_nx = 0, last_ny = 0;
  unsigned last_sw = 0, last_sh = 0;
  unsigned last_drawn = 0;
  bool last_idle = false;
  bool have_gpu_sync = false;

  void Reset() noexcept {
    frames = reuse = prep = scan = 0;
    prep_us = prep_max_us = 0;
    scan_us = scan_max_us = 0;
    draw_us = draw_max_us = 0;
    gpu_sync_us = 0;
    quads_ov = quads_tile = 0;
    uploads = 0;
    upload_bytes = 0;
    have_gpu_sync = false;
  }

  void AddUs(uint64_t us, uint64_t &sum, uint64_t &mx) noexcept {
    sum += us;
    if (us > mx)
      mx = us;
  }

  void Flush() noexcept {
    if (frames == 0)
      return;

    const auto elapsed = log_clock.Elapsed();
    const double sec =
      elapsed.count() > 0
      ? std::chrono::duration<double>(elapsed).count()
      : 2.0;
    const double fps = frames / sec;
    const unsigned prep_n = prep > 0 ? prep : 1;
    const double prep_avg_ms = (prep_us / 1000.0) / prep_n;
    const double draw_avg_ms = (draw_us / 1000.0) / frames;
    const double tiles_pf = quads_tile / double(frames);

    LogFmt("OpenGL: GPU DEM {:.1f}s frames={} ({:.0f}/s) idle={}",
           sec, frames, fps, last_idle ? 1 : 0);
    LogFmt("OpenGL: GPU DEM path reuse={} prep={} scan={}",
           reuse, prep, scan);
    LogFmt("OpenGL: GPU DEM cpu prep avg/max {:.2f}/{:.2f} ms  "
           "scan avg/max {:.2f}/{:.2f} ms  "
           "draw avg/max {:.2f}/{:.2f} ms",
           prep_avg_ms, prep_max_us / 1000.0,
           scan > 0 ? (scan_us / 1000.0) / scan : 0.0,
           scan_max_us / 1000.0,
           draw_avg_ms, draw_max_us / 1000.0);
    LogFmt("OpenGL: GPU DEM quads ov={} tiles={} (avg {:.1f}/frame) "
           "uploads={} ({:.1f} KiB)",
           quads_ov, quads_tile, tiles_pf,
           uploads, upload_bytes / 1024.0);
    LogFmt("OpenGL: GPU DEM q={} qe={} active={} grid={}x{} "
           "view={}x{} drawn={} gpu_sync={:.1f}ms",
           last_q, last_qe, last_active, last_nx, last_ny,
           last_sw, last_sh, last_drawn,
           have_gpu_sync ? gpu_sync_us / 1000.0 : -1.0);

    Reset();
    log_clock.Update();
  }
};

static GpuDemStats gpu_dem_stats;

static uint64_t
SteadyUsSince(std::chrono::steady_clock::time_point t0) noexcept
{
  return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - t0)
                    .count());
}

#endif

/** Keep slope neighbour sampling inside the height matrix. */
static void
ClampQuantisationEffectiveToMatrix(unsigned &quantisation_effective,
                                     UnsignedPoint2D matrix_size) noexcept
{
  if (quantisation_effective == 0)
    return;

  if (matrix_size.x <= 1 || matrix_size.y <= 1) {
    quantisation_effective = 0;
    return;
  }

  const unsigned max_step =
    std::min(matrix_size.x - 1, matrix_size.y - 1);
  if (quantisation_effective > max_step)
    quantisation_effective = max_step;
}

[[gnu::const]]
static unsigned
SafeMinusStep(unsigned pos, unsigned step) noexcept
{
  return std::min(step, pos);
}

[[gnu::const]]
static unsigned
SafePlusStep(unsigned pos, unsigned size, unsigned step) noexcept
{
  if (size <= 1 || pos >= size - 1)
    return 0;

  return std::min(step, size - 1 - pos);
}

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
                    MIX(32, color.Blue(), x));
  } else if (illum > 0) {
    // highlight to yellow
    int x = std::min(32, illum / 2);
    return RawColor(MIX(255, color.Red(), x),
                    MIX(255, color.Green(), x),
                    MIX(196, color.Blue(), x));
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

struct ColumnContourPending {
  unsigned until_row;
  RawColor color;
};

/**
 * Apply centered contour thickness expansion for a contour pixel.
 * Paints immediately into already-rendered pixels (above and left),
 * and sets deferred pending state for not-yet-rendered pixels (below
 * and right).
 *
 * @param tl top/left extend: contour_thickness / 2
 * @param br bottom/right extend: (contour_thickness - 1) / 2
 */
static inline void
ApplyContourExpansion(RawColor *p,
                      ptrdiff_t row_stride,
                      unsigned col, unsigned row,
                      unsigned width,
                      unsigned tl, unsigned br,
                      RawColor contour_color,
                      ColumnContourPending *pending) noexcept
{
  // Immediate: top-left block (current pixel + above and left)
  for (unsigned r = 0; r <= tl && r <= row; ++r)
    for (unsigned c = 0; c <= tl && c <= col; ++c)
      *(p - c - ptrdiff_t(r) * row_stride) = contour_color;

  if (br > 0) {
    // Immediate: top-right block (above current row, right of col)
    for (unsigned r = 1; r <= tl && r <= row; ++r)
      for (unsigned c = 1; c <= br && col + c < width; ++c)
        *(p + c - ptrdiff_t(r) * row_stride) = contour_color;

    // Deferred: pending for bottom portion and right side of
    // current row.
    const unsigned target_row = row + br;
    const unsigned col_start = col >= tl ? col - tl : 0;
    const unsigned col_end = std::min(col + br, width - 1);
    for (unsigned cx = col_start; cx <= col_end; ++cx)
      if (target_row > pending[cx].until_row) {
        pending[cx].until_row = target_row;
        pending[cx].color = contour_color;
      }
  }
}

RasterRenderer::RasterRenderer() noexcept = default;

RasterRenderer::~RasterRenderer() noexcept
{
  delete[] color_table;
  delete image;
  delete[] contour_column_base;
  delete[] contour_pending;
}

#ifdef ENABLE_OPENGL

[[gnu::pure]]
static unsigned
GetQuantisation() noexcept
{
  if (!IsSlowCPU() && !OpenGL::idle_terrain_quantisation)
    /* fast hosts: full resolution immediately (GPU hillshade and
       ScanMap are cheap enough without the idle ladder) */
    return 1;

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

  quantisation_pixels = std::max(GetQuantisation(), min_quantisation_pixels);
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
  // GeoPoint corresponding to the MapWindow center
  GeoPoint center = projection.ScreenToGeo(projection.GetScreenCenter());

  // Geographical edge length of one height matrix cell in meters
  if (quantisation_pixels < 1)
    quantisation_pixels = 1;

  pixel_size = quantisation_pixels / projection.GetScale();

  ApplySlopeQuantisation(quantisation_effective, pixel_size,
                         map.PixelDistance(center, 1));

#ifdef ENABLE_OPENGL
  const double bounds_scale = TerrainBoundsScale();
  bounds = projection.GetScreenBounds().Scale(bounds_scale);
  bounds.IntersectWith(map.GetBounds());

  UnsignedPoint2D matrix_size =
    (UnsignedPoint2D)projection.GetScreenSize()
    * static_cast<unsigned>(bounds_scale * 128.0f + 0.5f)
    / quantisation_pixels / 128;
  if (matrix_size.x == 0 || matrix_size.y == 0) {
    quantisation_effective = 0;
    return;
  }

  /* VC4 (and other GLES2 GPUs) reject textures larger than
     GL_MAX_TEXTURE_SIZE (often 2048).  q=1 at 1080p with
     BOUNDS_SCALE_FACTOR can request ~2880 wide → GL_INVALID_VALUE
     on TexSubImage and a black map. */
  const unsigned max_texture = OpenGL::max_texture_size > 0
    ? OpenGL::max_texture_size
    : OpenGL::DEFAULT_MAX_TEXTURE_SIZE;
  if (matrix_size.x > max_texture || matrix_size.y > max_texture) {
    const double scale = std::min(double(max_texture) / matrix_size.x,
                                  double(max_texture) / matrix_size.y);
    const unsigned clamped_x =
      std::max(1u, unsigned(matrix_size.x * scale));
    const unsigned clamped_y =
      std::max(1u, unsigned(matrix_size.y * scale));
    LogFmt("Terrain: clamp matrix {}x{} -> {}x{} (max texture {})",
           matrix_size.x, matrix_size.y, clamped_x, clamped_y,
           max_texture);
    matrix_size = {clamped_x, clamped_y};
  }

  height_matrix.Fill(map, bounds, matrix_size, true);

  ClampQuantisationEffectiveToMatrix(quantisation_effective,
                                     height_matrix.GetSize());

  last_quantisation_pixels = quantisation_pixels;
#else
  height_matrix.Fill(map, projection, quantisation_pixels, true);

  ClampQuantisationEffectiveToMatrix(quantisation_effective,
                                     height_matrix.GetSize());
#endif
}

void
RasterRenderer::FillGradient(UnsignedPoint2D size,
                             int16_t min_h, int16_t max_h,
                             bool vertical) noexcept
{
  height_matrix.FillGradient(size, min_h, max_h, vertical);
  quantisation_effective = 1;
}

void
RasterRenderer::GenerateImage(bool do_shading,
                              unsigned height_scale,
                              int contrast, int brightness,
                              const Angle sunazimuth,
                              unsigned contour_spacing) noexcept
{
  // At extreme zoom out, terrain features are too small to be meaningful;
  // disable both slope shading and contours.
  ClampQuantisationEffectiveToMatrix(quantisation_effective,
                                     height_matrix.GetSize());
  if (quantisation_effective == 0) {
    do_shading = false;
    contour_spacing = 0;
  }

  // Convert spacing to scale, with scale=16: effectively no contours
  unsigned contour_height_scale = 16;
  if (contour_spacing > 0) {
    unsigned s = 0;
    while ((1u << s) < contour_spacing)
      ++s;
    contour_height_scale = s;
  }

  // Compute contour width, aiming for 0.75 units (=3/4 of one 80 dpi pixel)
  contour_thickness = contour_height_scale < 16
    ? std::max(1u,
               Layout::ScalePenWidth(1u * 768u)
               / (quantisation_pixels * 1024u))
    : 1;

#ifdef ENABLE_OPENGL
  height_scale_for_draw = height_scale;
  shading_for_draw = do_shading;
  {
    const unsigned q = std::max(1u, quantisation_effective);
    const unsigned q_sq = q * q;
    const unsigned max_hsf = std::max(1u, 8192u / q_sq);
    height_slope_factor_for_draw =
      std::clamp(static_cast<unsigned>(pixel_size), 1u, max_hsf);
  }

  if (!use_cpu_hillshade && OpenGL::hillshade_shader != nullptr &&
      !has_alpha &&
      height_matrix.GetSize().x > 0 && height_matrix.GetSize().y > 0) {
    gpu_dem_tiles = false;
    SetContourSpacing(contour_spacing);
    UploadHeightTexture();
    UploadRampTexture();
    shader_hillshade = true;
    return;
  }

  shader_hillshade = false;
  gpu_dem_tiles = false;
#endif

  if (image == nullptr ||
      height_matrix.GetSize().x > image->GetSize().width ||
      height_matrix.GetSize().y > image->GetSize().height) {
    delete image;
    image = new RawBitmap(PixelSize{height_matrix.GetSize()});

    delete[] contour_column_base;
    contour_column_base = new unsigned char[height_matrix.GetSize().x];

    delete[] contour_pending;
    contour_pending =
      new ColumnContourPending[height_matrix.GetSize().x];
  }

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
  const ptrdiff_t row_stride =
    image->GetNextRow(dest) - dest;
  const unsigned matrix_width = height_matrix.GetSize().x;
  const unsigned contour_tl = contour_thickness / 2;
  const unsigned contour_br = (contour_thickness - 1) / 2;

  for (unsigned y = height_matrix.GetSize().y; y > 0; --y) {
    RawColor *p = dest;
    dest = image->GetNextRow(dest);

    const unsigned current_row =
      height_matrix.GetSize().y - y;

    unsigned contour_row_base = ContourInterval(*src, contour_height_scale);
    unsigned char *contour_this_column_base = contour_column_base;

    for (unsigned x = matrix_width; x > 0; --x) {
      const auto e = *src++;
      const unsigned col = matrix_width - x;

      // Check if pixel is claimed by a prior contour expansion
      if (contour_br > 0 &&
          contour_pending[col].until_row > 0 &&
          current_row <= contour_pending[col].until_row)
        [[unlikely]] {
        *p++ = contour_pending[col].color;
        if (!e.IsSpecial()) {
          const unsigned ci = ContourInterval(
            std::max(0, (int)e.GetValue()),
            contour_height_scale);
          *contour_this_column_base =
            contour_row_base = ci;
        }
        contour_this_column_base++;
        continue;
      }

      if (!e.IsSpecial()) [[likely]] {
        unsigned h = std::max(0, (int)e.GetValue());

        const unsigned contour_interval =
          ContourInterval(h, contour_height_scale);

        h = std::min(254u, h >> height_scale);
        if (contour_interval != contour_row_base ||
            contour_interval != *contour_this_column_base) [[unlikely]] {
          const RawColor contour_color =
            oColorBuf[(int)h - 64 * 256];

          if (contour_thickness > 1)
            ApplyContourExpansion(
              p, row_stride,
              col, current_row, matrix_width,
              contour_tl, contour_br,
              contour_color, contour_pending);
          else
            *p = contour_color;

          ++p;
          *contour_this_column_base = contour_row_base = contour_interval;
        } else {
          *p++ = oColorBuf[h];
        }
      } else if (e.IsWater()) {
        // we're in the water, so look up the color for water
        *p++ = oColorBuf[255];
      } else {
        /* outside the terrain file bounds */
        *p++ = oColorBuf[255];
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

void
RasterRenderer::GenerateSlopeImage(unsigned height_scale,
                                   int contrast,
                                   const int sx, const int sy, const int sz,
                                   const unsigned contour_height_scale) noexcept
{
  const UnsignedPoint2D matrix_size = height_matrix.GetSize();
  ClampQuantisationEffectiveToMatrix(quantisation_effective, matrix_size);
  if (quantisation_effective == 0)
    return;

  const unsigned q_sq = quantisation_effective * quantisation_effective;
  const unsigned max_height_slope_factor =
    std::max(1u, 8192u / q_sq);
  const unsigned height_slope_factor =
    std::clamp(static_cast<unsigned>(pixel_size), 1u,
               /* upper limit avoids integer overflows in the "mag"
                  formula; it effectively limits "dd2" so calculating
                  its square will not overflow */
               max_height_slope_factor);

  const auto *src = height_matrix.GetData();
  const RawColor *oColorBuf = color_table + 64 * 256;

  RawColor *dest = image->GetTopRow();
  const ptrdiff_t row_stride =
    image->GetNextRow(dest) - dest;
  const unsigned matrix_width = matrix_size.x;
  const unsigned contour_tl = contour_thickness / 2;
  const unsigned contour_br = (contour_thickness - 1) / 2;

  for (unsigned y = 0; y < matrix_size.y; ++y) {
    const unsigned row_plus_index =
      SafePlusStep(y, matrix_size.y, quantisation_effective);
    const unsigned row_plus_offset = matrix_size.x * row_plus_index;

    const unsigned row_minus_index =
      SafeMinusStep(y, quantisation_effective);
    const unsigned row_minus_offset = matrix_size.x * row_minus_index;

    const unsigned p31 = row_plus_index + row_minus_index;

    RawColor *p = dest;
    dest = image->GetNextRow(dest);

    unsigned contour_row_base = ContourInterval(*src, contour_height_scale);
    unsigned char *contour_this_column_base = contour_column_base;

    for (unsigned x = 0; x < matrix_size.x; ++x, ++src) {
      const auto e = *src;

      // Check if pixel is claimed by a prior contour expansion
      if (contour_br > 0 &&
          contour_pending[x].until_row > 0 &&
          y <= contour_pending[x].until_row) [[unlikely]] {
        *p++ = contour_pending[x].color;
        if (!e.IsSpecial()) {
          const unsigned ci = ContourInterval(
            std::max(0, (int)e.GetValue()),
            contour_height_scale);
          *contour_this_column_base =
            contour_row_base = ci;
        }
        contour_this_column_base++;
        continue;
      }

      if (!e.IsSpecial()) [[likely]] {
        unsigned h = std::max(0, (int)e.GetValue());

        const unsigned contour_interval =
          ContourInterval(h, contour_height_scale);

        h = std::min(254u, h >> height_scale);

        const unsigned column_plus_index =
          SafePlusStep(x, matrix_size.x, quantisation_effective);
        const unsigned column_minus_index =
          SafeMinusStep(x, quantisation_effective);

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

          const RawColor contour_color =
            oColorBuf[int(h) - 64 * 256];

          *contour_this_column_base++ = contour_row_base = contour_interval;

          if (contour_thickness > 1)
            ApplyContourExpansion(
              p, row_stride,
              x, y, matrix_width,
              contour_tl, contour_br,
              contour_color, contour_pending);
          else
            *p = contour_color;

          ++p;
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
        /* outside the terrain file bounds */
        *p++ = oColorBuf[255];
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
  has_alpha = false;

  if (color_table == nullptr)
    color_table = new RawColor[256 * 128];

#ifdef ENABLE_OPENGL
  ramp_texture_dirty = true;
#endif

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
                          interp_levels);

        color = TerrainShading(mag, color2);
      }

      color_table[i + (mag + 64) * 256] = color;
    }
  }
}

/**
 * Shade the given RGBA color according to the illumination value.
 * Similar to TerrainShading but preserves alpha channel.
 */
static constexpr RawColor
TerrainShadingAlpha(const int illum, RGBA8Color color) noexcept
{
  if (illum == -64) {
    // brown color mixed in for contours
    return RawColor(MIX(100, color.Red(), 64),
                    MIX(70, color.Green(), 64),
                    MIX(26, color.Blue(), 64),
                    color.Alpha());
  } else if (illum < 0) {
    // shadow to blue
    int x = std::min(63, -illum);
    return RawColor(MIX(0, color.Red(), x),
                    MIX(0, color.Green(), x),
                    MIX(32, color.Blue(), x),
                    color.Alpha());
  } else if (illum > 0) {
    // highlight to yellow
    int x = std::min(32, illum / 2);
    return RawColor(MIX(255, color.Red(), x),
                    MIX(255, color.Green(), x),
                    MIX(196, color.Blue(), x),
                    color.Alpha());
  } else
    return RawColor(color.Red(), color.Green(), color.Blue(), color.Alpha());
}

void
RasterRenderer::PrepareColorTableAlpha(const ColorRamp *color_ramp,
                                       bool do_water,
                                       unsigned height_scale,
                                       int interp_levels) noexcept
{
  has_alpha = true;

  if (color_table == nullptr)
    color_table = new RawColor[256 * 128];

#ifdef ENABLE_OPENGL
  ramp_texture_dirty = true;
#endif

  for (int i = 0; i < 256; i++) {
    for (int mag = -64; mag < 64; mag++) {
      RawColor color;

      if (i == 255) {
        if (do_water) {
          // water colours (opaque)
          color = RawColor(85, 160, 255, 0xff);
        } else {
          // fully transparent
          color = RawColor(255, 255, 255, 0x00);
        }
      } else {
        const RGBA8Color color2 =
          ColorRampLookupAlpha(i << height_scale, color_ramp,
                               interp_levels);

        color = TerrainShadingAlpha(mag, color2);
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

  // reset deferred contour expansion state
  std::fill_n(contour_pending, height_matrix.GetSize().x,
              ColumnContourPending{});
}

#ifdef ENABLE_OPENGL

static void
FillRampRgba(uint8_t *dest, const RawColor *table) noexcept
{
  for (unsigned i = 0; i < 256 * 128; ++i) {
#ifdef GREYSCALE
    const uint8_t y = table[i].value.GetLuminosity();
    *dest++ = y;
    *dest++ = y;
    *dest++ = y;
    *dest++ = 255;
#elif defined(USE_RGB565)
    const uint16_t v = table[i].value.GetNativeValue();
    *dest++ = uint8_t((v >> 8) & 0xf8);
    *dest++ = uint8_t((v >> 3) & 0xfc);
    *dest++ = uint8_t((v << 3) & 0xf8);
    *dest++ = 255;
#else
    *dest++ = table[i].value.Red();
    *dest++ = table[i].value.Green();
    *dest++ = table[i].value.Blue();
    *dest++ = table[i].alpha;
#endif
  }
}

void
RasterRenderer::SetSunFromAzimuth(Angle sunazimuth, int brightness,
                                  int contrast) noexcept
{
  const Angle fudgeelevation = Angle::Degrees(10) +
    Angle::Degrees(80.0 / 255.0) * brightness;

  sun_sx = (int)(255 * fudgeelevation.fastcosine() *
                 -sunazimuth.fastsine());
  sun_sy = (int)(255 * fudgeelevation.fastcosine() *
                 -sunazimuth.fastcosine());
  sun_sz = (int)(255 * fudgeelevation.fastsine());
  contrast_for_draw = contrast;
}

void
RasterRenderer::SetContourSpacing(unsigned contour_spacing) noexcept
{
  if (contour_spacing == 0) {
    contour_div_for_draw = 0;
    return;
  }

  unsigned s = 0;
  while ((1u << s) < contour_spacing)
    ++s;

  contour_div_for_draw = s >= 16 ? 0 : (1u << s);
}

void
RasterRenderer::UploadHeightLATexture(const void *data, PixelSize ps,
                                      std::unique_ptr<GLTexture> &dest) noexcept
{
  static_assert(std::endian::native == std::endian::little);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if (dest == nullptr || dest->GetSize() != ps) {
    dest = std::make_unique<GLTexture>(GL_LUMINANCE_ALPHA, ps,
                                       GL_LUMINANCE_ALPHA,
                                       GL_UNSIGNED_BYTE, data);
  } else {
    dest->Bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ps.width, ps.height,
                    GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
  }

  dest->Bind();
  /* Packed int16 in L/A: decode, then filter in the shader. */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void
RasterRenderer::UploadHeightTexture() noexcept
{
  const auto sz = height_matrix.GetSize();
  UploadHeightLATexture(height_matrix.GetData(),
                        PixelSize{int(sz.x), int(sz.y)},
                        height_texture);
}

void
RasterRenderer::SyncGpuDemTileTextures(const RasterMap &map) noexcept
{
  const auto &cache = map.GetTileCache();
  const unsigned nx = cache.GetTileCountX();
  const unsigned ny = cache.GetTileCountY();
  const unsigned n = nx * ny;
  dem_tile_grid = {nx, ny};

  tile_tex_active = 0;
  tile_tex_dropped = false;

  if (tile_textures.size() != n) {
    tile_textures.clear();
    tile_textures.resize(n);
    tile_starts.assign(n, {});
    tile_ends.assign(n, {});
    /* Grid rebuild drops cached tile textures. */
    tile_tex_dropped = true;
  }

  const RasterBuffer &overview = map.GetOverview();
  if (overview.IsDefined()) {
    const auto osz = overview.GetSize();
    const PixelSize ps{int(osz.x), int(osz.y)};
      if (overview_texture == nullptr || overview_texture->GetSize() != ps) {
        UploadHeightLATexture(overview.GetData(), ps, overview_texture);
        gpu_dem_stats.uploads++;
        gpu_dem_stats.upload_bytes +=
          unsigned(ps.width) * unsigned(ps.height) * 2u;
      }
  }

  for (unsigned y = 0; y < ny; ++y) {
    for (unsigned x = 0; x < nx; ++x) {
      const unsigned i = y * nx + x;
      const RasterTile &tile = cache.GetTile(x, y);
      if (!tile.IsLoaded() || !tile.buffer.IsDefined()) {
        if (tile_textures[i]) {
          tile_textures[i].reset();
          tile_tex_dropped = true;
        }
        continue;
      }

      ++tile_tex_active;
      tile_starts[i] = tile.start;
      tile_ends[i] = tile.end;
      const auto tsz = tile.buffer.GetSize();
      const PixelSize ps{int(tsz.x), int(tsz.y)};
      auto &tex = tile_textures[i];
      if (tex == nullptr || tex->GetSize() != ps) {
        UploadHeightLATexture(tile.buffer.GetData(), ps, tex);
        gpu_dem_stats.uploads++;
        gpu_dem_stats.upload_bytes +=
          unsigned(ps.width) * unsigned(ps.height) * 2u;
      }
    }
  }
}

bool
RasterRenderer::PrepareGpuDemTiles(const RasterMap &map,
                                   const WindowProjection &projection,
                                   bool do_shading,
                                   unsigned height_scale,
                                   unsigned contour_spacing,
                                   bool allow_incremental) noexcept
{
  const auto t0 = std::chrono::steady_clock::now();
  gpu_dem_tiles = false;

  if (use_cpu_hillshade ||
      OpenGL::hillshade_shader == nullptr ||
      has_alpha)
    return false;

  SyncGpuDemTileTextures(map);
  if (tile_tex_active == 0 && overview_texture == nullptr)
    return false;

  dem_map_bounds = map.GetBounds();
  dem_projection = map.GetProjection();

  /* Same overscan + resolution policy as ScanMap(). */
  const GeoPoint center = projection.ScreenToGeo(projection.GetScreenCenter());
  if (quantisation_pixels < 1)
    quantisation_pixels = 1;
  pixel_size = quantisation_pixels / projection.GetScale();
  gpu_dem_cell_meters = std::max(1.0, map.PixelDistance(center, 1));
  ApplySlopeQuantisation(quantisation_effective, pixel_size,
                         gpu_dem_cell_meters);

  last_quantisation_pixels = quantisation_pixels;

  /* DrawGpuDemTiles uses the overview for coverage; keep an overscan
     bounds only so Generate() can skip this call while panning. */
  if (!allow_incremental || !bounds.IsValid() || tile_tex_dropped) {
    bounds = projection.GetScreenBounds().Scale(BOUNDS_SCALE_FACTOR);
    bounds.IntersectWith(dem_map_bounds);
    if (!bounds.IsValid())
      return false;
  }

  UnsignedPoint2D matrix_size =
    (UnsignedPoint2D)projection.GetScreenSize() / quantisation_pixels;
  if (matrix_size.x < 2)
    matrix_size.x = 2;
  if (matrix_size.y < 2)
    matrix_size.y = 2;
  ClampQuantisationEffectiveToMatrix(quantisation_effective, matrix_size);

  height_scale_for_draw = height_scale;
  shading_for_draw = do_shading && quantisation_effective > 0;
  SetContourSpacing(contour_spacing);
  UploadRampTexture();

  gpu_dem_tiles = true;
  shader_hillshade = true;

  gpu_dem_stats.prep++;
  gpu_dem_stats.AddUs(SteadyUsSince(t0),
                      gpu_dem_stats.prep_us, gpu_dem_stats.prep_max_us);

  static bool logged_gpu_dem = false;
  if (!logged_gpu_dem) {
    logged_gpu_dem = true;
    LogFormat("OpenGL: GPU DEM tiles (overview + %u fine)",
              tile_tex_active);
  }

  return true;
}

void
RasterRenderer::GpuDemNoteReuse() noexcept
{
  gpu_dem_stats.reuse++;
}

void
RasterRenderer::GpuDemNoteScanMap(unsigned cpu_us) noexcept
{
  gpu_dem_stats.scan++;
  gpu_dem_stats.AddUs(cpu_us, gpu_dem_stats.scan_us,
                      gpu_dem_stats.scan_max_us);
}

void
RasterRenderer::UploadRampTexture() noexcept
{
  assert(color_table != nullptr);

  if (ramp_texture != nullptr && !ramp_texture_dirty)
    return;

  constexpr unsigned n = 256 * 128 * 4;
  if (ramp_rgba == nullptr)
    ramp_rgba = std::make_unique<uint8_t[]>(n);

  FillRampRgba(ramp_rgba.get(), color_table);

  constexpr PixelSize ramp_size{256, 128};

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if (ramp_texture == nullptr) {
    ramp_texture = std::make_unique<GLTexture>(GL_RGBA, ramp_size,
                                               GL_RGBA, GL_UNSIGNED_BYTE,
                                               ramp_rgba.get());
  } else {
    ramp_texture->Bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    ramp_size.width, ramp_size.height,
                    GL_RGBA, GL_UNSIGNED_BYTE, ramp_rgba.get());
  }

  ramp_texture->Bind();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  ramp_texture_dirty = false;
}

double
RasterRenderer::GpuDemHeightSlopeFactor() const noexcept
{
  /* Meters per DEM sample: n0/n2 is then the true slope. */
  return gpu_dem_cell_meters;
}

void
RasterRenderer::DrawHillshadeQuad(const WindowProjection &projection,
                                  const GLTexture &height_tex,
                                  const GeoPoint &nw, const GeoPoint &ne,
                                  const GeoPoint &sw, const GeoPoint &se,
                                  double height_slope_factor,
                                  float alpha) const noexcept
{
  assert(ramp_texture != nullptr);

  const BulkPixelPoint vertices[] = {
    projection.GeoToScreen(nw),
    projection.GeoToScreen(ne),
    projection.GeoToScreen(sw),
    projection.GeoToScreen(se),
  };

  const ScopeVertexPointer vp(vertices);

  glActiveTexture(GL_TEXTURE0);
  const_cast<GLTexture &>(height_tex).Bind();
  /* GLTexture::Configure() defaults to LINEAR; packed int16 heights
     must stay NEAREST or decode-and-filter in the shader is skipped. */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glActiveTexture(GL_TEXTURE1);
  ramp_texture->Bind();
  glActiveTexture(GL_TEXTURE0);

  OpenGL::hillshade_shader->Use();

  const PixelSize allocated = height_tex.GetAllocatedSize();
  const PixelSize size = height_tex.GetSize();
  const GLfloat x1 = GLfloat(size.width) / allocated.width;
  const GLfloat y1 = GLfloat(size.height) / allocated.height;
  const GLfloat coord[] = {
    0, 0,
    x1, 0,
    0, y1,
    x1, y1,
  };

  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, coord);

  glUniform3f(OpenGL::hillshade_sun,
              GLfloat(sun_sx), GLfloat(sun_sy), GLfloat(sun_sz));
  /* GPU DEM samples real 90 m posts, not interpolated screen pixels.
     The CPU contrast curve is too shallow for Jura-scale slopes at
     default brightness; scale it so relief stays visible. */
  const float contrast = gpu_dem_tiles
    ? float(contrast_for_draw) * 2.5f
    : float(contrast_for_draw);
  glUniform1f(OpenGL::hillshade_contrast, contrast);
  glUniform1f(OpenGL::hillshade_height_slope_factor,
              GLfloat(height_slope_factor));
  glUniform1f(OpenGL::hillshade_height_div,
              GLfloat(1u << height_scale_for_draw));
  glUniform1f(OpenGL::hillshade_do_shading, shading_for_draw ? 1.f : 0.f);
  glUniform2f(OpenGL::hillshade_height_texel,
              1.f / GLfloat(allocated.width),
              1.f / GLfloat(allocated.height));
  glUniform1f(OpenGL::hillshade_contour_div,
              GLfloat(contour_div_for_draw));
  {
    const float span_x =
      std::hypot(float(vertices[1].x - vertices[0].x),
                 float(vertices[1].y - vertices[0].y));
    const float span_y =
      std::hypot(float(vertices[2].x - vertices[0].x),
                 float(vertices[2].y - vertices[0].y));
    /* Screen pixels per DEM texel (shader isoline width). */
    const unsigned tw = std::max(1u, size.width);
    const unsigned th = std::max(1u, size.height);
    const GLfloat csx = span_x / float(tw);
    const GLfloat csy = span_y / float(th);
    glUniform2f(OpenGL::hillshade_contour_step, csx, csy);
  }

  if (alpha < 1.0f) {
    const GLBlend blend(alpha);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  } else {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
}

void
RasterRenderer::DrawHillshade(const WindowProjection &projection,
                              float alpha) const noexcept
{
  assert(bounds.IsValid());
  assert(height_texture != nullptr);
  assert(ramp_texture != nullptr);

  DrawHillshadeQuad(projection, *height_texture,
                    bounds.GetNorthWest(), bounds.GetNorthEast(),
                    bounds.GetSouthWest(), bounds.GetSouthEast(),
                    height_slope_factor_for_draw, alpha);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);
  OpenGL::solid_shader->Use();
}

void
RasterRenderer::DrawGpuDemTiles(const WindowProjection &projection,
                                float alpha) const noexcept
{
  assert(ramp_texture != nullptr);
  assert(dem_map_bounds.IsValid());

  const auto t0 = std::chrono::steady_clock::now();
  const GeoBounds screen = projection.GetScreenBounds();
  const double hsf = GpuDemHeightSlopeFactor();
  unsigned tiles_drawn = 0;
  unsigned ov_drawn = 0;

  if (overview_texture) {
    const double overview_hsf =
      std::max(1.0, hsf * double(1u << RasterTraits::OVERVIEW_BITS));
    DrawHillshadeQuad(projection, *overview_texture,
                      dem_map_bounds.GetNorthWest(),
                      dem_map_bounds.GetNorthEast(),
                      dem_map_bounds.GetSouthWest(),
                      dem_map_bounds.GetSouthEast(),
                      overview_hsf, alpha);
    ov_drawn = 1;
  }

  const unsigned nx = dem_tile_grid.x;
  const unsigned ny = dem_tile_grid.y;
  if (nx > 0 && ny > 0 &&
      tile_textures.size() == nx * ny &&
      tile_starts.size() == nx * ny) {
    for (unsigned i = 0; i < nx * ny; ++i) {
      if (!tile_textures[i])
        continue;

      const RasterLocation start = tile_starts[i];
      const RasterLocation end = tile_ends[i];
      if (end.x <= start.x || end.y <= start.y)
        continue;

      const GeoPoint nw = dem_projection.UnprojectCoarse(start);
      const GeoPoint ne = dem_projection.UnprojectCoarse(
        SignedRasterLocation(int(end.x), int(start.y)));
      const GeoPoint sw = dem_projection.UnprojectCoarse(
        SignedRasterLocation(int(start.x), int(end.y)));
      const GeoPoint se = dem_projection.UnprojectCoarse(end);

      const GeoBounds tb(nw, se);
      if (!tb.IsValid() || !tb.Overlaps(screen))
        continue;

      DrawHillshadeQuad(projection, *tile_textures[i],
                        nw, ne, sw, se,
                        hsf, alpha);
      ++tiles_drawn;
    }
  }

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);
  OpenGL::solid_shader->Use();

  const uint64_t cpu_us = SteadyUsSince(t0);
  gpu_dem_stats.frames++;
  gpu_dem_stats.AddUs(cpu_us, gpu_dem_stats.draw_us,
                      gpu_dem_stats.draw_max_us);
  gpu_dem_stats.quads_ov += ov_drawn;
  gpu_dem_stats.quads_tile += tiles_drawn;
  gpu_dem_stats.last_q = quantisation_pixels;
  gpu_dem_stats.last_qe = quantisation_effective;
  gpu_dem_stats.last_active = tile_tex_active;
  gpu_dem_stats.last_nx = nx;
  gpu_dem_stats.last_ny = ny;
  gpu_dem_stats.last_drawn = tiles_drawn;
  const auto view = projection.GetScreenSize();
  gpu_dem_stats.last_sw = view.width;
  gpu_dem_stats.last_sh = view.height;
  gpu_dem_stats.last_idle = IsUserIdle(750);

  if (!gpu_dem_stats.log_clock.IsDefined())
    gpu_dem_stats.log_clock.Update();
  else if (gpu_dem_stats.log_clock.Check(GPU_DEM_STATS_PERIOD)) {
    const auto g0 = std::chrono::steady_clock::now();
    glFinish();
    gpu_dem_stats.gpu_sync_us = SteadyUsSince(g0);
    gpu_dem_stats.have_gpu_sync = true;
    const GLenum err = glGetError();
    if (err != GL_NO_ERROR)
      LogFmt("OpenGL: GPU DEM glGetError=0x{:x}", unsigned(err));
    gpu_dem_stats.Flush();
  }
}

#endif

void
RasterRenderer::Draw([[maybe_unused]] Canvas &canvas,
                     const WindowProjection &projection,
                     [[maybe_unused]] bool transparent_white,
                     [[maybe_unused]] float alpha) const noexcept
{
#ifdef ENABLE_OPENGL
  if (gpu_dem_tiles && ramp_texture && dem_map_bounds.IsValid()) {
    /* Overview first, then loaded fine tiles (covers holes). */
    if (dem_map_bounds.Overlaps(projection.GetScreenBounds()))
      DrawGpuDemTiles(projection, alpha);
    return;
  }

  if (!bounds.IsValid() || !bounds.Overlaps(projection.GetScreenBounds()))
    return;

  if (shader_hillshade && height_texture && ramp_texture) {
    DrawHillshade(projection, alpha);
    return;
  }

  if (image == nullptr)
    return;

  const ScopeTextureConstantAlpha blend(has_alpha, alpha);

  DrawGeoBitmap(*image,
                PixelSize{height_matrix.GetSize()},
                bounds,
                projection);
#else
  image->StretchTo(PixelSize{height_matrix.GetSize()},
                   canvas, projection.GetScreenSize(),
                   transparent_white, has_alpha, alpha);
#endif
}
