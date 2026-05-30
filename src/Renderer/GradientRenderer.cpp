// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GradientRenderer.hpp"
#include "Asset.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "Math/Angle.hpp"
#include "Math/FastMath.hpp"
#include "Math/Point2D.hpp"
#include "util/Macros.hpp"

#include <algorithm>

[[gnu::const]]
static Color
BlendTowardFace(Color shadow, Color face,
                unsigned step, unsigned steps) noexcept
{
  if (steps <= 1 || step >= steps - 1)
    return step >= steps - 1 ? face : shadow;

#ifdef GREYSCALE
  const uint8_t l = shadow.GetLuminosity()
    + (int(face.GetLuminosity()) - int(shadow.GetLuminosity()))
      * int(step) / int(steps - 1);
  return Color(l);
#else
  const uint8_t r = shadow.Red()
    + (int(face.Red()) - int(shadow.Red()))
      * int(step) / int(steps - 1);
  const uint8_t g = shadow.Green()
    + (int(face.Green()) - int(shadow.Green()))
      * int(step) / int(steps - 1);
  const uint8_t b = shadow.Blue()
    + (int(face.Blue()) - int(shadow.Blue()))
      * int(step) / int(steps - 1);
  return Color(r, g, b);
#endif
}

#if defined(EYE_CANDY) && defined(ENABLE_OPENGL)

#include "ui/canvas/opengl/Shaders.hpp"
#include "ui/canvas/opengl/Program.hpp"
#include "ui/canvas/opengl/VertexPointer.hpp"
#include "ui/canvas/opengl/Color.hpp"
#include "util/AllocatedArray.hxx"
#include "util/StaticArray.hxx"

static constexpr unsigned CIRCLE_SEGS = 64;
static constexpr unsigned MAX_ARC_ANGLES = CIRCLE_SEGS + 2;
static constexpr unsigned GL_RINGS = 24;

[[gnu::const]]
static PixelPoint
CirclePoint(int radius, unsigned angle) noexcept
{
  assert(angle < ISINETABLE.size());

  return PixelPoint(ISINETABLE[angle] * radius / 1024,
                    -ISINETABLE[(angle + INT_QUARTER_CIRCLE) & INT_ANGLE_MASK] * radius / 1024);
}

[[gnu::const]]
static PixelPoint
CirclePoint(PixelPoint p, int radius, unsigned angle) noexcept
{
  return p + CirclePoint(radius, angle);
}

/** Collect arc sample angles (same spacing as Canvas::DrawAnnulus). */
static unsigned
CollectArcAngles(unsigned istart, unsigned iend,
                 StaticArray<unsigned, MAX_ARC_ANGLES> &angles) noexcept
{
  angles.clear();
  angles.push_back(istart);

  const unsigned ilast = istart < iend ? iend : iend + INT_ANGLE_RANGE;
  for (unsigned i = istart + INT_ANGLE_RANGE / CIRCLE_SEGS; i < ilast;
       i += INT_ANGLE_RANGE / CIRCLE_SEGS) {
    const unsigned angle = i & INT_ANGLE_MASK;
    if (angles.back() != angle)
      angles.push_back(angle);
  }

  if (angles.back() != iend)
    angles.push_back(iend);

  return angles.size();
}

[[gnu::const]]
static Color
ShadowColorAtRadius(unsigned radius, unsigned inner_radius,
                    unsigned outer_radius, Color shadow, Color face,
                    unsigned fade_depth) noexcept
{
  const unsigned edge_dist = std::min(radius - inner_radius,
                                      outer_radius - radius);
  if (edge_dist >= fade_depth)
    return face;

  return BlendTowardFace(shadow, face, edge_dist, fade_depth);
}

/**
 * Radial vertex-colour gradient on an annulus arc using a regular quad mesh.
 * Only the fade zones should be drawn — not the full band centre.
 */
static void
DrawAnnulusShadowGradientMesh(PixelPoint center,
                              unsigned mesh_inner, unsigned mesh_outer,
                              unsigned band_inner, unsigned band_outer,
                              Angle arc_start, Angle arc_end,
                              Color face_color, Color shadow,
                              unsigned fade_depth) noexcept
{
  if (mesh_outer <= mesh_inner)
    return;

  const int istart = NATIVE_TO_INT(arc_start.Native());
  const int iend = NATIVE_TO_INT(arc_end.Native());

  StaticArray<unsigned, MAX_ARC_ANGLES> angles;
  const unsigned n_angles = CollectArcAngles(istart, iend, angles);
  if (n_angles < 2)
    return;

  const unsigned band = mesh_outer - mesh_inner;
  const unsigned n_rings = std::min(GL_RINGS, std::max(2u, band)) + 1;
  const unsigned n_vertices = n_rings * n_angles;
  if (n_vertices < 4)
    return;

  static AllocatedArray<BulkPixelPoint> vertices;
  static AllocatedArray<Color> colors;
  static AllocatedArray<GLushort> indices;

  vertices.GrowDiscard(n_vertices);
  colors.GrowDiscard(n_vertices);

  for (unsigned r = 0; r < n_rings; ++r) {
    const unsigned radius = mesh_inner + band * r / (n_rings - 1);
    const Color color = ShadowColorAtRadius(radius, band_inner, band_outer,
                                            shadow, face_color, fade_depth);

    for (unsigned a = 0; a < n_angles; ++a) {
      const unsigned idx = r * n_angles + a;
      vertices[idx] = CirclePoint(center, int(radius), angles[a]);
      colors[idx] = color;
    }
  }

  const unsigned n_strip_quads = (n_rings - 1) * (n_angles - 1);
  const unsigned idx_count = n_strip_quads * 6;
  indices.GrowDiscard(idx_count);

  unsigned ii = 0;
  for (unsigned r = 0; r + 1 < n_rings; ++r) {
    for (unsigned a = 0; a + 1 < n_angles; ++a) {
      const GLushort i0 = GLushort(r * n_angles + a);
      const GLushort i1 = GLushort(r * n_angles + a + 1);
      const GLushort i2 = GLushort((r + 1) * n_angles + a + 1);
      const GLushort i3 = GLushort((r + 1) * n_angles + a);

      indices[ii++] = i0;
      indices[ii++] = i1;
      indices[ii++] = i2;
      indices[ii++] = i0;
      indices[ii++] = i2;
      indices[ii++] = i3;
    }
  }

  OpenGL::solid_shader->Use();

  const ScopeVertexPointer vp(vertices.data());
  const ScopeColorPointer cp(colors.data());

  glDrawElements(GL_TRIANGLES, idx_count, GL_UNSIGNED_SHORT, indices.data());
}

static void
DrawAnnulusEdgeShadowFadeGL(PixelPoint center,
                            unsigned inner_radius, unsigned outer_radius,
                            Angle arc_start, Angle arc_end,
                            Color face_color, Color shadow,
                            unsigned fade_depth) noexcept
{
  DrawAnnulusShadowGradientMesh(center, inner_radius,
                                inner_radius + fade_depth,
                                inner_radius, outer_radius,
                                arc_start, arc_end,
                                face_color, shadow, fade_depth);

  if (fade_depth * 2 >= outer_radius - inner_radius)
    return;

  DrawAnnulusShadowGradientMesh(center, outer_radius - fade_depth,
                                outer_radius,
                                inner_radius, outer_radius,
                                arc_start, arc_end,
                                face_color, shadow, fade_depth);
}

#endif

#if !defined(EYE_CANDY) || !defined(ENABLE_OPENGL)

static void
DrawAnnulusEdgeShadowFadeBanded(Canvas &canvas, PixelPoint center,
                                unsigned inner_radius, unsigned outer_radius,
                                Angle arc_start, Angle arc_end,
                                Color face_color, Color shadow,
                                unsigned fade_depth) noexcept
{
  static constexpr unsigned MAX_STEPS = 4;
  const unsigned steps = std::min(MAX_STEPS, fade_depth);

  canvas.SelectNullPen();

  for (unsigned i = 0; i < steps; ++i) {
    const unsigned r0 = inner_radius + i * fade_depth / steps;
    const unsigned r1 = inner_radius + (i + 1) * fade_depth / steps;
    if (r1 <= r0)
      continue;

    canvas.Select(Brush(BlendTowardFace(shadow, face_color, i, steps)));
    canvas.DrawAnnulus(center, r0, r1, arc_start, arc_end);
  }

  for (unsigned i = 0; i < steps; ++i) {
    const unsigned r1 = outer_radius - i * fade_depth / steps;
    const unsigned r0 = outer_radius - (i + 1) * fade_depth / steps;
    if (r1 <= r0)
      continue;

    canvas.Select(Brush(BlendTowardFace(shadow, face_color, i, steps)));
    canvas.DrawAnnulus(center, r0, r1, arc_start, arc_end);
  }
}

#endif

void
DrawVerticalGradient([[maybe_unused]] Canvas &canvas, const PixelRect &rc,
                     [[maybe_unused]] Color top_color, [[maybe_unused]] Color bottom_color, [[maybe_unused]] Color fallback_color)
{
#if defined(EYE_CANDY) && defined(ENABLE_OPENGL)
  const BulkPixelPoint vertices[] = {
    rc.GetTopLeft(),
    rc.GetTopRight(),
    rc.GetBottomLeft(),
    rc.GetBottomRight(),
  };

  const ScopeVertexPointer vp(vertices);

  const Color colors[] = {
    top_color,
    top_color,
    bottom_color,
    bottom_color,
  };

  const ScopeColorPointer cp(colors);

  static_assert(ARRAY_SIZE(vertices) == ARRAY_SIZE(colors),
                "Array size mismatch");

  glDrawArrays(GL_TRIANGLE_STRIP, 0, ARRAY_SIZE(vertices));
#else
  canvas.DrawFilledRectangle(rc, fallback_color);
#endif
}

void
DrawBandedVerticalGradient(Canvas &canvas, const PixelRect &rc,
                           Color top_color, Color bottom_color)
{
  const int height = rc.GetHeight();
  if (height <= 0)
    return;

  constexpr unsigned N_BANDS = 64;
  for (unsigned i = 0; i < N_BANDS; i++) {
    const int y0 = rc.top + (int)((unsigned)height * i / N_BANDS);
    const int y1 = rc.top + (int)((unsigned)height * (i + 1) / N_BANDS);
#ifdef GREYSCALE
    const uint8_t l = top_color.GetLuminosity()
      + (int(bottom_color.GetLuminosity()) - int(top_color.GetLuminosity()))
        * (int)i / (int)(N_BANDS - 1);
    canvas.DrawFilledRectangle(PixelRect{PixelPoint{rc.left, y0},
                                         PixelPoint{rc.right, y1}},
                               Color(l));
#else
    const uint8_t r = top_color.Red()
      + (int(bottom_color.Red()) - int(top_color.Red()))
        * (int)i / (int)(N_BANDS - 1);
    const uint8_t g = top_color.Green()
      + (int(bottom_color.Green()) - int(top_color.Green()))
        * (int)i / (int)(N_BANDS - 1);
    const uint8_t b = top_color.Blue()
      + (int(bottom_color.Blue()) - int(top_color.Blue()))
        * (int)i / (int)(N_BANDS - 1);
    canvas.DrawFilledRectangle(PixelRect{PixelPoint{rc.left, y0},
                                         PixelPoint{rc.right, y1}},
                               Color(r, g, b));
#endif
  }
}

void
DrawAnnulusEdgeShadowFade([[maybe_unused]] Canvas &canvas, PixelPoint center,
                          unsigned inner_radius, unsigned outer_radius,
                          Angle arc_start, Angle arc_end,
                          Color face_color) noexcept
{
  const unsigned band = outer_radius - inner_radius;
  if (band < 2)
    return;

  static constexpr unsigned SHADOW_FADE_BAND_PERCENT = 22;
  const unsigned fade_depth = band * SHADOW_FADE_BAND_PERCENT / 100;
  if (fade_depth == 0)
    return;

  const Color shadow = IsDithered() ? COLOR_BLACK : DarkColor(face_color);

#if defined(EYE_CANDY) && defined(ENABLE_OPENGL)
  DrawAnnulusEdgeShadowFadeGL(center, inner_radius, outer_radius,
                              arc_start, arc_end, face_color, shadow,
                              fade_depth);
#else
  DrawAnnulusEdgeShadowFadeBanded(canvas, center, inner_radius, outer_radius,
                                  arc_start, arc_end, face_color, shadow,
                                  fade_depth);
#endif
}
