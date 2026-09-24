// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/opengl/System.hpp"

class GLProgram;

namespace OpenGL {

/**
 * A shader that draws a solid color (#Attribute::COLOR).
 */
extern GLProgram *solid_shader;
extern GLint solid_projection, solid_modelview, solid_translate;

/**
 * A shader that copies the texture.
 */
extern GLProgram *texture_shader;
extern GLint texture_projection, texture_texture, solid_translate;

/**
 * A shader that copies the inverted texture.
 */
extern GLProgram *invert_shader;
extern GLint invert_projection, invert_texture, invert_translate;

/**
 * A shader that copies the texture's alpha channel, but replaces
 * the color (#Attribute::COLOR).
 */
extern GLProgram *alpha_shader;
extern GLint alpha_projection, alpha_texture, alpha_translate;

/**
 * A shader that multiplies the texture with #Attribute::COLOR.
 */
extern GLProgram *combine_texture_shader;
extern GLint combine_texture_projection, combine_texture_texture,
  combine_texture_translate;

/**
 * A shader that draws dashed lines (#Pen::Style).
 */
extern GLProgram *dashed_shader;
extern GLint dashed_projection, dashed_translate,
  dashed_resolution, dashed_start, dashed_period, dashed_ratio;

/**
 * A shader that draws a circle outline.
 */
extern GLProgram *circle_outline_shader;
extern GLint circle_outline_projection, circle_outline_translate,
  circle_outline_center, circle_outline_radius1, circle_outline_radius2,
  circle_outline_color;

/**
 * A shader that draws a filled circle.
 */
extern GLProgram *filled_circle_shader;
extern GLint filled_circle_projection, filled_circle_translate,
  filled_circle_center, filled_circle_radius1, filled_circle_radius2,
  filled_circle_color1, filled_circle_color2;

/**
 * A shader that draws a line with round ends and a smooth edge.  Each
 * segment is a quad around it; the #TEXCOORD attribute holds the
 * segment's two end points (x1, y1, x2, y2), and every fragment
 * within the #RADIUS attribute of the segment gets
 * #round_line_color.  The edge fades out over #round_line_softness
 * pixels.  A segment whose ends are the same point draws a dot.
 *
 * Fragments covered less than #round_line_min_coverage are
 * discarded; see #RoundLines for how this keeps overlapping
 * translucent segments from blending twice.
 */
extern GLProgram *round_line_shader;
extern GLint round_line_projection, round_line_translate,
  round_line_softness, round_line_min_coverage, round_line_color;

/**
 * Throws on error.
 */
void InitShaders();

void DeinitShaders() noexcept;

void UpdateShaderProjectionMatrix() noexcept;

void
UpdateShaderTranslate() noexcept;

} // namespace OpenGL
