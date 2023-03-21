// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Terrain/HeightMatrix.hpp"

#ifdef ENABLE_OPENGL
#include "Geo/GeoBounds.hpp"
#endif

static constexpr unsigned NUM_COLOR_RAMP_LEVELS = 13;

class Angle;
class Canvas;
class RasterMap;
class WindowProjection;
class RawBitmap;
struct RawColor;
struct ColorRamp;

#ifdef ENABLE_OPENGL
class GLTexture;
#endif

class RasterRenderer {
  /** screen dimensions in coarse pixels */
  unsigned quantisation_pixels = 2;

#ifdef ENABLE_OPENGL
  /**
   * The value of #quantisation_pixels that was used in the last
   * ScanMap() call.
   */
  unsigned last_quantisation_pixels = -1;
#endif

  /**
   * Step size used for slope calculations.  Slope shading is disabled
   * when this attribute is 0.
   */
  unsigned quantisation_effective;

#ifdef ENABLE_OPENGL
  /**
   * The area that was rendered previously into the #HeightMatrix and
   * the #RawBitmap.  This attribute is used to decide whether the
   * texture has to be redrawn.
   */
  GeoBounds bounds = GeoBounds::Invalid();
#endif

  HeightMatrix height_matrix;
  RawBitmap *image = nullptr;

  unsigned char *contour_column_base = nullptr;

  double pixel_size;

  RawColor *color_table = nullptr;

public:
  RasterRenderer();
  ~RasterRenderer();

  RasterRenderer(const RasterRenderer &) = delete;
  RasterRenderer &operator=(const RasterRenderer &) = delete;

  const HeightMatrix &GetHeightMatrix() const {
    return height_matrix;
  }

  unsigned GetWidth() const {
    return height_matrix.GetWidth();
  }

  unsigned GetHeight() const {
    return height_matrix.GetHeight();
  }

#ifdef ENABLE_OPENGL
  void Invalidate() {
    bounds.SetInvalid();
  }

  /**
   * Calculate a new #quantisation_pixels value.
   *
   * @return true if the new #quantisation_pixels value is smaller
   * than the previous one (redraw needed)
   */
  bool UpdateQuantisation();

  const GeoBounds &GetBounds() const {
    return bounds;
  }

  const GLTexture &BindAndGetTexture() const;
#endif

  /**
   * Fills the color_table array with precomputed colors for 256 height and
   * 64 illumination levels. This is used to speed up the rendering by
   * preventing the same color calculations over and over again.
   */
  void PrepareColorTable(const ColorRamp *color_ramp, bool do_water,
                         unsigned height_scale, int interp_levels);

  /**
   * Scan the map and fill the height matrix.
   */
  void ScanMap(const RasterMap &map, const WindowProjection &projection);

  /**
   * Convert the height matrix into the image.
   */
  void GenerateImage(bool do_shading,
                     unsigned height_scale, int contrast, int brightness,
                     const Angle sunazimuth,
                     bool do_contour);

  const RawBitmap &GetImage() const {
    return *image;
  }

  void Draw(Canvas &canvas, const WindowProjection &projection,
            bool transparent_white=false) const;

protected:
  /**
   * Convert the height matrix into the image, without shading.
   */
  void GenerateUnshadedImage(unsigned height_scale,
                             const unsigned contour_height_scale);

  /**
   * Convert the height matrix into the image, with slope shading.
   */
  void GenerateSlopeImage(unsigned height_scale, int contrast,
                          const int sx, const int sy, const int sz,
                          const unsigned contour_height_scale);

  /**
   * Convert the height matrix into the image, with slope shading.
   */
  void GenerateSlopeImage(unsigned height_scale,
                          int contrast, int brightness,
                          const Angle sunazimuth,
                          const unsigned contour_height_scale);

private:

  void ContourStart(const unsigned contour_height_scale);
};
