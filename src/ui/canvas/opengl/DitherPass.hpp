// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Size.hpp"
#include "ui/opengl/System.hpp"

struct BulkPixelPoint;
class Canvas;
class GLTexture;

namespace OpenGL {

/**
 * Master switch for the OpenGL e-ink display profile.  When enabled:
 * - #HasColors() and #UseGreyscaleDisplay() are false/true
 * - #IsDithered() and #HasEPaper() are true
 * - Terrain is dithered at draw time (#DrawGeoBitmapWithDither())
 * - #ApplyGreyscalePass() converts the full frame to greyscale without dither
 */
extern bool enable_dither_pass;

enum class DitherAlgorithm {
  /** Greyscale only on terrain, no dither pattern. */
  NONE,
  BAYER,
  SIERRA_LITE,
  ERROR_DIFFUSION = SIERRA_LITE,
};

extern DitherAlgorithm dither_algorithm;

struct DitherPassSettings {
  float snap_high = 0.93f;
  float snap_low = 0.07f;
  float gamma = 0.88f;
};

extern DitherPassSettings dither_pass_settings;

void ApplyDitherUniforms() noexcept;

/**
 * Draw a geo-referenced terrain bitmap with dither at screen resolution.
 */
void DrawGeoBitmapWithDither(const GLTexture &texture, PixelSize bitmap_size,
                             const BulkPixelPoint (&vertices)[4],
                             const GLfloat texcoord[8]);

/**
 * Capture the default framebuffer and redraw it in greyscale (no dither).
 * Call from TopCanvas::Flip() before eglSwapBuffers().
 */
void ApplyGreyscalePass(Canvas &canvas);

} // namespace OpenGL
