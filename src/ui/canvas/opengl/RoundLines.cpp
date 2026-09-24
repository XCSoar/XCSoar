// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RoundLines.hpp"
#include "Scope.hpp"
#include "Shaders.hpp"
#include "Program.hpp"
#include "VertexPointer.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/dim/BulkPoint.hpp"

#include <algorithm>
#include <cmath>

/**
 * The number of passes for translucent colours, see Draw().
 */
static constexpr unsigned COVERAGE_STEPS = 16;

void
RoundLines::AddSegment(FloatPoint2D a, FloatPoint2D b, float radius)
{
  /* a quad which covers everything within the radius and the soft
     edge, plus one pixel */
  const float margin = std::max(radius, 0.f) + softness / 2 + 1;

  FloatPoint2D d = b - a;
  const float length = std::hypot(d.x, d.y);
  d = length > 0 ? d * (margin / length) : FloatPoint2D{margin, 0};
  const FloatPoint2D n{-d.y, d.x};

  const FloatPoint2D p0 = a - d + n, p1 = a - d - n;
  const FloatPoint2D p2 = b + d + n, p3 = b + d - n;

  for (const auto &p : {p0, p1, p2, p2, p1, p3})
    vertices.push_back({p, a, b, radius});
}

void
RoundLines::AddLine(std::span<const BulkPixelPoint> points,
                    float radius)
{
  for (std::size_t i = 1; i < points.size(); ++i)
    AddSegment(FloatPoint2D(points[i - 1].x, points[i - 1].y),
               FloatPoint2D(points[i].x, points[i].y),
               radius);
}

template<typename F>
void
RoundLines::WithVertices(Color color, F &&draw) const noexcept
{
  OpenGL::round_line_shader->Use();
  glUniform1f(OpenGL::round_line_softness, softness);
  color.Uniform(OpenGL::round_line_color);

  const auto &v = vertices.front();
  const GLsizei stride = sizeof(v);

  ScopeVertexPointer vp;
  vp.Update(GL_FLOAT, stride, &v.position);

  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 4, GL_FLOAT, GL_FALSE,
                        stride, &v.a);
  glEnableVertexAttribArray(OpenGL::Attribute::RADIUS);
  glVertexAttribPointer(OpenGL::Attribute::RADIUS, 1, GL_FLOAT, GL_FALSE,
                        stride, &v.radius);

  draw(GLsizei(vertices.size()));

  glDisableVertexAttribArray(OpenGL::Attribute::RADIUS);
  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
}

/**
 * Does the current framebuffer have a stencil buffer?  Android may
 * choose an EGL config without one.
 */
static bool
HaveStencilBuffer() noexcept
{
  GLint bits = 0;
  glGetIntegerv(GL_STENCIL_BITS, &bits);
  return bits > 0;
}

/**
 * Where segments overlap, a translucent colour must not be blended
 * twice, or the joints would show as darker beads: the stencil
 * buffer lets only the first segment paint a pixel.  The passes go
 * from full to faint coverage, so that the pixel is painted by a
 * segment which covers it nearly as much as the one covering it
 * most; the difference is less than 1/#COVERAGE_STEPS, too little to
 * show as bands in a soft edge.
 *
 * The first stencil bit records the painted pixels, #MASK_BIT (from
 * DrawMask()) the ones to leave alone.
 */
void
RoundLines::Draw(Color color, bool masked) const noexcept
{
  if (vertices.empty())
    return;

  const ScopeAlphaBlend alpha_blend;

  WithVertices(color, [&](GLsizei n){
    if ((color.IsOpaque() && !masked) || !HaveStencilBuffer()) {
      /* painting a pixel twice does no harm to an opaque colour;
         without a stencil buffer, a translucent one shows its joints
         and ignores the mask, which is still better than blending
         all passes on top of each other */
      glUniform1f(OpenGL::round_line_min_coverage, 0);
      glDrawArrays(GL_TRIANGLES, 0, n);
      return;
    }

    const GLEnable<GL_STENCIL_TEST> stencil_test;

    /* clear only the first bit, keep the mask */
    glStencilMask(1);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_EQUAL, 0, masked ? 1 | MASK_BIT : 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);

    for (unsigned i = COVERAGE_STEPS; i-- > 0;) {
      glUniform1f(OpenGL::round_line_min_coverage,
                  float(i) / COVERAGE_STEPS);
      glDrawArrays(GL_TRIANGLES, 0, n);
    }

    glStencilMask(~0u);
  });
}

void
RoundLines::DrawMask() const noexcept
{
  if (vertices.empty())
    return;

  WithVertices(COLOR_BLACK, [](GLsizei n){
    const GLEnable<GL_STENCIL_TEST> stencil_test;

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilMask(MASK_BIT);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, MASK_BIT, MASK_BIT);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    /* the pixels at least half covered */
    glUniform1f(OpenGL::round_line_min_coverage, 0.5f);
    glDrawArrays(GL_TRIANGLES, 0, n);

    glStencilMask(~0u);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  });
}
