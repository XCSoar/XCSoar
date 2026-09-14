// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BoxShadowRenderer.hpp"
#include "ui/dim/Rect.hpp"

#ifdef ENABLE_OPENGL

#include "Math/Point2D.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/opengl/Program.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#include "ui/canvas/opengl/Shaders.hpp"
#include "ui/canvas/opengl/VertexPointer.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>

namespace {

/**
 * How far the opaque core of the shadow reaches beyond the box, in
 * virtual points.
 */
constexpr int SHADOW_SPREAD = 6;

/**
 * The width of the blurred transition, in virtual points.  It is
 * centered on the edge of the shadow's shape, i.e. the shadow fades
 * out over the last SHADOW_BLUR/2 points and reaches
 * SHADOW_SPREAD+SHADOW_BLUR/2 beyond the box.
 */
constexpr int SHADOW_BLUR = 32;

/**
 * The opacity of the black shadow where it is darkest.
 */
constexpr uint8_t SHADOW_ALPHA = 115;

/**
 * The number of segments each corner arc of a contour is approximated
 * with.
 */
constexpr unsigned CORNER_SEGMENTS = 8;

/**
 * The number of vertices of one contour: four corner arcs, each
 * starting and ending on one of the straight edges.
 */
constexpr unsigned CONTOUR_VERTICES = 4 * (CORNER_SEGMENTS + 1);

/**
 * The maximum number of contours; this limits both the amount of work
 * per shadow and the size of the index buffer.
 */
constexpr unsigned MAX_CONTOURS = 33;

static_assert(MAX_CONTOURS * CONTOUR_VERTICES <= 0x10000,
              "Too many vertices for 16 bit indices");

/**
 * The largest mesh #MAX_CONTOURS can produce: one triangle fan filling
 * the innermost contour, plus two triangles per vertex between each
 * pair of adjacent contours.
 */
constexpr unsigned MAX_VERTICES = MAX_CONTOURS * CONTOUR_VERTICES;
constexpr unsigned MAX_INDICES = (CONTOUR_VERTICES - 2) * 3
  + (MAX_CONTOURS - 1) * CONTOUR_VERTICES * 6;

/**
 * The opacity of a Gaussian blur across its transition, approximated
 * with a smoothstep(): 1 at the inner end (x=0), 0.5 exactly on the
 * edge of the shadow shape (x=0.5) and 0 at the outer end (x=1).
 */
constexpr float
BlurOpacity(float x) noexcept
{
  x = std::clamp(x, 0.f, 1.f);
  return 1.f - x * x * (3.f - 2.f * x);
}

/**
 * The unit circle offsets of one quarter arc, from (1,0) to (0,1).
 */
const auto quarter_arc = []{
  std::array<FloatPoint2D, CORNER_SEGMENTS + 1> result{};
  for (unsigned i = 0; i <= CORNER_SEGMENTS; ++i) {
    const float angle = float(std::numbers::pi / 2) * i / CORNER_SEGMENTS;
    result[i] = {std::cos(angle), std::sin(angle)};
  }
  return result;
}();

/**
 * Emit the contour of all points which have the given distance to the
 * four corner points of #centers - i.e. a rounded rectangle.
 *
 * All contours of one shadow share the same #centers, which makes the
 * triangles between two of them annular sectors: their corner values
 * lie on a plane, so both halves of each quad interpolate the same
 * gradient.  Contours built as nested sharp-cornered rectangles would
 * instead meet in a mitre at each corner, and because that mitre is
 * wider than the straight bands, it shows up as a bright diagonal
 * streak extending each corner.
 *
 * The contour is emitted clockwise, starting on the left edge, and
 * always consists of #CONTOUR_VERTICES vertices, so that two contours
 * can be stitched together with a ring of triangles.
 */
void
AppendContour(FloatPoint2D *dest, const PixelRect &centers,
              float radius) noexcept
{
  const float left = centers.left, top = centers.top;
  const float right = centers.right, bottom = centers.bottom;

  for (const auto i : quarter_arc)
    /* top left, from the left edge to the top edge */
    *dest++ = {left - radius * i.x, top - radius * i.y};

  for (const auto i : quarter_arc)
    /* top right, from the top edge to the right edge */
    *dest++ = {right + radius * i.y, top - radius * i.x};

  for (const auto i : quarter_arc)
    /* bottom right, from the right edge to the bottom edge */
    *dest++ = {right + radius * i.x, bottom + radius * i.y};

  for (const auto i : quarter_arc)
    /* bottom left, from the bottom edge to the left edge */
    *dest++ = {left - radius * i.y, bottom + radius * i.x};
}

} // anonymous namespace

#endif /* ENABLE_OPENGL */

void
DrawBoxShadow([[maybe_unused]] const PixelRect &rc) noexcept
{
#ifdef ENABLE_OPENGL
  /* the shape which gets blurred: the box, inflated by the spread */
  PixelRect shape = rc;
  shape.Grow(Layout::VptScale(SHADOW_SPREAD));

  const int blur = Layout::VptScale(SHADOW_BLUR);

  /* the blur transition reaches this far outside and inside of the
     shape's edge */
  const int outer = blur / 2;
  const int inner = std::min<int>(outer,
                                  std::min(shape.GetWidth(),
                                           shape.GetHeight()) / 2);

  /* the corner arcs of all contours are centered on these four
     points; the innermost contour collapses onto them, and the
     outermost is #inner+#outer away */
  const PixelRect centers{
    shape.left + inner, shape.top + inner,
    shape.right - inner, shape.bottom - inner,
  };

  /* one contour every two pixels is plenty for a smooth gradient */
  const unsigned n_intervals = std::clamp<unsigned>((outer + inner) / 2,
                                                    1, MAX_CONTOURS - 1);
  const unsigned n_contours = n_intervals + 1;

  /* both buffers are sized for the largest mesh we can produce, so
     that this noexcept function never needs to allocate */
  std::array<FloatPoint2D, MAX_VERTICES> vertices;
  std::array<Color, MAX_VERTICES> colors;

  for (unsigned i = 0; i < n_contours; ++i) {
    const float radius = float(outer + inner) * i / n_intervals;

    AppendContour(&vertices[i * CONTOUR_VERTICES], centers, radius);

    /* the opacity depends on the distance to the shape's edge, which
       this contour crosses at radius==inner */
    const float opacity = BlurOpacity((radius - inner + blur / 2.f) / blur);
    const Color color =
      COLOR_BLACK.WithAlpha(uint8_t(std::lround(SHADOW_ALPHA * opacity)));
    std::fill_n(&colors[i * CONTOUR_VERTICES], CONTOUR_VERTICES, color);
  }

  std::array<GLushort, MAX_INDICES> indices;
  unsigned n_indices = 0;

  /* fill the innermost contour with a triangle fan */
  for (unsigned i = 1; i + 1 < CONTOUR_VERTICES; ++i) {
    indices[n_indices++] = 0;
    indices[n_indices++] = i;
    indices[n_indices++] = i + 1;
  }

  /* stitch adjacent contours together with a ring of triangles; the
     vertex colors make OpenGL interpolate the blur gradient */
  for (unsigned i = 0; i < n_intervals; ++i) {
    const unsigned a = i * CONTOUR_VERTICES;
    const unsigned b = a + CONTOUR_VERTICES;

    for (unsigned j = 0; j < CONTOUR_VERTICES; ++j) {
      const unsigned j2 = (j + 1) % CONTOUR_VERTICES;

      indices[n_indices++] = a + j;
      indices[n_indices++] = b + j;
      indices[n_indices++] = a + j2;

      indices[n_indices++] = b + j;
      indices[n_indices++] = b + j2;
      indices[n_indices++] = a + j2;
    }
  }

  assert(n_indices <= indices.size());

  const ScopeAlphaBlend alpha_blend;
  const ScopeVertexPointer vp(vertices.data());
  const ScopeColorPointer cp(colors.data());

  OpenGL::solid_shader->Use();
  glDrawElements(GL_TRIANGLES, GLsizei(n_indices),
                 GL_UNSIGNED_SHORT, indices.data());
#endif
}
