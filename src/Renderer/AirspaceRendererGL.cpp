// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef ENABLE_OPENGL

#include "AirspaceRenderer.hpp"
#include "AirspaceRendererSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"
#include "MapWindow/MapCanvas.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceWarningCopy.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"
#include "ui/canvas/opengl/Scope.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

/**
 * Stencil bits used by the airspace renderers.
 */
static constexpr GLuint FILL_STENCIL = 1;
static constexpr GLuint OUTLINE_STENCIL = 2;
static constexpr GLuint POLYGON_STENCIL = 4;
static constexpr GLint EVEN_ODD_STENCIL_BITS = 3;

static_assert(POLYGON_STENCIL < (1u << EVEN_ODD_STENCIL_BITS));

/**
 * The maximum number of vertices handled by the allocation-free even-odd
 * renderer.  Larger clipped polygons fall back to the generic renderer.
 */
static constexpr unsigned MAX_AIRSPACE_VERTICES = 64000;
// Six vertices draw the edge rectangle and three draw its miter or bevel.
static constexpr unsigned MAX_PADDING_VERTICES = MAX_AIRSPACE_VERTICES * 9;

static_assert(MAX_AIRSPACE_VERTICES + 2 <=
              unsigned(std::numeric_limits<GLsizei>::max()));
static_assert(MAX_PADDING_VERTICES <=
              unsigned(std::numeric_limits<GLsizei>::max()));

/**
 * The OpenGL airspace renderer draws serially in one context.  Reusing this
 * fixed workspace avoids per-frame allocation for the even-odd and padding
 * geometry added by this renderer.
 */
struct AirspaceWorkspace {
  std::array<FloatPoint2D, MAX_AIRSPACE_VERTICES + 2> fan;
  std::array<FloatPoint2D, MAX_AIRSPACE_VERTICES> points;
  std::array<FloatPoint2D, MAX_AIRSPACE_VERTICES> normals;
  std::array<FloatPoint2D, MAX_PADDING_VERTICES> triangles;
};

static AirspaceWorkspace airspace_workspace;

static constexpr float PADDING_MITER_LIMIT = 4;

/**
 * Build a wide closed line with miter joins and bounded bevel joins as
 * independent triangles.
 *
 * The generic OpenGL line strip builder changes the strip orientation at
 * acute bends.  The explicit geometry keeps the join choice stable while
 * panning.  Normal corners use the familiar mitered appearance; only an
 * extreme miter or a reversal is bevelled.
 */
static unsigned
BuildPaddingLoop(const BulkPixelPoint *src, unsigned src_size,
                 float line_width) noexcept
{
  if (src_size < 3 || src_size > MAX_AIRSPACE_VERTICES)
    return 0;

  auto &points = airspace_workspace.points;
  auto &normals = airspace_workspace.normals;

  unsigned size = 0;
  for (unsigned i = 0; i < src_size; ++i) {
    if (size == 0 ||
        points[size - 1].x != src[i].x || points[size - 1].y != src[i].y)
      points[size++] = {float(src[i].x), float(src[i].y)};
  }

  if (size > 1 && points[0] == points[size - 1])
    --size;

  if (size < 3)
    return 0;

  const float half_width = line_width * 0.5f;
  for (unsigned i = 0; i < size; ++i) {
    const auto &a = points[i];
    const auto &b = points[(i + 1) % size];
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float length = std::hypot(dx, dy);
    if (length <= 0)
      return 0;

    const float scale = half_width / length;
    normals[i] = {-dy * scale, dx * scale};
  }

  unsigned n = 0;
  const auto append = [&n](FloatPoint2D p) noexcept {
    airspace_workspace.triangles[n++] = p;
  };

  for (unsigned i = 0; i < size; ++i) {
    const auto &a = points[i];
    const auto &b = points[(i + 1) % size];
    const auto &normal = normals[i];
    const auto a_left = a + normal;
    const auto a_right = a - normal;
    const auto b_left = b + normal;
    const auto b_right = b - normal;

    append(a_left);
    append(a_right);
    append(b_left);
    append(b_left);
    append(a_right);
    append(b_right);

    const auto &previous = points[(i + size - 1) % size];
    const auto previous_direction = a - previous;
    const auto next_direction = b - a;
    const auto &previous_normal = normals[(i + size - 1) % size];
    const auto bend = CrossProduct(previous_normal, normal);
    if (bend == 0)
      continue;

    const float outside = bend > 0 ? -1.f : 1.f;
    const auto previous_offset = a + previous_normal * outside;
    const auto next_offset = a + normal * outside;
    const auto denominator = CrossProduct(previous_direction, next_direction);

    if (std::fabs(denominator) > 1e-6f) {
      const float t = CrossProduct(next_offset - previous_offset,
                                   next_direction) / denominator;
      const auto miter = previous_offset + previous_direction * t;
      const float miter_length = std::hypot(miter.x - a.x, miter.y - a.y);
      if (miter_length <= PADDING_MITER_LIMIT * half_width) {
        append(previous_offset);
        append(miter);
        append(next_offset);
        continue;
      }
    }

    append(a);
    append(previous_offset);
    append(next_offset);
  }

  return n;
}

/**
 * Fill a possibly degenerate polygon using the even-odd rule.
 *
 * Clipping and projection to integer screen coordinates may turn an otherwise
 * valid airspace polygon into a self-touching polygon.  Ear clipping cannot
 * triangulate such a polygon, but toggling a stencil bit for every triangle in
 * a fan leaves that bit set exactly where an odd number of triangles overlap.
 * This also handles self-intersections consistently.
 *
 * The caller must have selected the fill brush, enabled stencil testing and
 * initialized polygon_stencil to zero.  The temporary bit is cleared again
 * before returning.
 */
static void
DrawEvenOddPolygon(Canvas &canvas, const BulkPixelPoint *points,
                   unsigned num_points, GLuint polygon_stencil,
                   GLuint stencil_value,
                   GLuint stencil_mask) noexcept
{
  auto min_x = points[0].x;
  auto max_x = points[0].x;
  auto min_y = points[0].y;
  auto max_y = points[0].y;
  for (unsigned i = 1; i < num_points; ++i) {
    min_x = std::min(min_x, points[i].x);
    max_x = std::max(max_x, points[i].x);
    min_y = std::min(min_y, points[i].y);
    max_y = std::max(max_y, points[i].y);
  }

  /* Don't use a boundary vertex as the fan origin.  An acute first vertex
     would make every triangle converge at the least numerically stable part
     of the polygon.  A sub-pixel origin near the bounds center keeps narrow
     corners local to their two adjacent triangles. */
  auto &fan = airspace_workspace.fan;
  fan[0] = {
    (float(min_x) + float(max_x)) * 0.5f + 0.25f,
    (float(min_y) + float(max_y)) * 0.5f + 0.375f,
  };
  for (unsigned i = 0; i < num_points; ++i)
    fan[i + 1] = {float(points[i].x), float(points[i].y)};
  fan[num_points + 1] = fan[1];

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glStencilFunc(GL_ALWAYS, polygon_stencil, polygon_stencil);
  glStencilMask(polygon_stencil);
  glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
  canvas.DrawFilledTriangleFan(fan.data(), num_points + 2);

  // Extend the cover by one pixel so its edge rules also clear every stencil
  // fragment written along the polygon's maximum coordinates.
  const BulkPixelPoint bounds[] = {
    {int(min_x) - 1, int(min_y) - 1},
    {int(max_x) + 1, int(min_y) - 1},
    {int(max_x) + 1, int(max_y) + 1},
    {int(min_x) - 1, int(max_y) + 1},
  };

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_EQUAL, stencil_value | polygon_stencil,
                stencil_mask | polygon_stencil);
  glStencilMask(polygon_stencil);
  glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
  canvas.DrawTriangleFan(bounds, 4);
}

/** Restore the OpenGL state changed by a fill-enabled airspace pass. */
class GLStateGuard {
  GLboolean color_write_mask[4];
  GLint stencil_write_mask;
  GLint stencil_func, stencil_ref, stencil_value_mask;
  GLint stencil_fail, stencil_pass_depth_fail, stencil_pass_depth_pass;
  GLint blend_source_rgb, blend_destination_rgb;
  GLint blend_source_alpha, blend_destination_alpha;
  const GLboolean stencil_test_enabled;
  const GLboolean blend_enabled;

public:
  GLStateGuard() noexcept
    :stencil_test_enabled(glIsEnabled(GL_STENCIL_TEST)),
     blend_enabled(glIsEnabled(GL_BLEND))
  {
    glGetBooleanv(GL_COLOR_WRITEMASK, color_write_mask);
    glGetIntegerv(GL_STENCIL_WRITEMASK, &stencil_write_mask);
    glGetIntegerv(GL_STENCIL_FUNC, &stencil_func);
    glGetIntegerv(GL_STENCIL_REF, &stencil_ref);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, &stencil_value_mask);
    glGetIntegerv(GL_STENCIL_FAIL, &stencil_fail);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &stencil_pass_depth_fail);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &stencil_pass_depth_pass);
    glGetIntegerv(GL_BLEND_SRC_RGB, &blend_source_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &blend_destination_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_source_alpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_destination_alpha);
  }

  ~GLStateGuard() noexcept {
    glColorMask(color_write_mask[0], color_write_mask[1],
                color_write_mask[2], color_write_mask[3]);
    glStencilMask(static_cast<GLuint>(stencil_write_mask));
    glStencilFunc(static_cast<GLenum>(stencil_func), stencil_ref,
                  static_cast<GLuint>(stencil_value_mask));
    glStencilOp(static_cast<GLenum>(stencil_fail),
                static_cast<GLenum>(stencil_pass_depth_fail),
                static_cast<GLenum>(stencil_pass_depth_pass));
    glBlendFuncSeparate(static_cast<GLenum>(blend_source_rgb),
                        static_cast<GLenum>(blend_destination_rgb),
                        static_cast<GLenum>(blend_source_alpha),
                        static_cast<GLenum>(blend_destination_alpha));

    if (stencil_test_enabled)
      glEnable(GL_STENCIL_TEST);
    else
      glDisable(GL_STENCIL_TEST);

    if (blend_enabled)
      glEnable(GL_BLEND);
    else
      glDisable(GL_BLEND);
  }

  GLStateGuard(const GLStateGuard &) = delete;
  GLStateGuard &operator=(const GLStateGuard &) = delete;
};

class AirspaceVisitorRenderer final
  : protected MapCanvas
{
  const AirspaceLook &look;
  const AirspaceWarningCopy &warning_manager;
  const AirspaceRendererSettings &settings;
  const bool use_even_odd;
  const bool use_outline_stencil;
  unsigned num_padding_vertices = 0;

public:
  AirspaceVisitorRenderer(Canvas &_canvas, const WindowProjection &_projection,
                          const AirspaceLook &_look,
                          const AirspaceWarningCopy &_warnings,
                          const AirspaceRendererSettings &_settings,
                          bool _use_even_odd, bool _use_outline_stencil)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(1.1)),
     look(_look), warning_manager(_warnings), settings(_settings),
     use_even_odd(_use_even_odd), use_outline_stencil(_use_outline_stencil)
  {
    if (use_outline_stencil || use_even_odd) {
      glStencilMask(0xff);
      glClear(GL_STENCIL_BUFFER_BIT);
    }

    if (settings.fill_mode != AirspaceRendererSettings::FillMode::NONE)
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

private:
  void VisitCircle(const AirspaceCircle &airspace) {
    const auto type_or_class = airspace.GetTypeOrClass();
    AirspaceClass as_type_or_class = settings.classes[type_or_class].display
      ? type_or_class : airspace.GetClass();
    const AirspaceClassRendererSettings &class_settings =
      settings.classes[as_type_or_class];
    const AirspaceClassLook &class_look = look.classes[as_type_or_class];

    auto screen_center =
      projection.GeoToScreen(airspace.GetReferenceLocation());
    unsigned screen_radius =
      projection.GeoToScreenDistance(airspace.GetRadius());

    if (!warning_manager.IsAcked(airspace) &&
        class_settings.fill_mode !=
        AirspaceClassRendererSettings::FillMode::NONE) {
      const GLEnable<GL_STENCIL_TEST> stencil;
      const GLEnable<GL_BLEND> blend;
      SetupInterior(airspace);
      if (warning_manager.HasWarning(airspace) ||
          warning_manager.IsInside(airspace) ||
          look.thick_pen.GetWidth() >= 2 * screen_radius ||
          class_settings.fill_mode ==
          AirspaceClassRendererSettings::FillMode::ALL) {
        // fill whole circle
        canvas.DrawCircle(screen_center, screen_radius);
      } else {
        // draw a ring inside the circle
        Color color = class_look.fill_color;
        Pen pen_donut(look.thick_pen.GetWidth() / 2, color.WithAlpha(90));
        canvas.SelectHollowBrush();
        canvas.Select(pen_donut);
        canvas.DrawCircle(screen_center,
                          screen_radius - look.thick_pen.GetWidth() / 4);
      }
    }

    // draw outline
    DrawOutline(airspace, screen_center, screen_radius);
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
    const auto type_or_class = airspace.GetTypeOrClass();
    AirspaceClass as_type_or_class = settings.classes[type_or_class].display
      ? type_or_class : airspace.GetClass();
    if (!PreparePolygon(airspace.GetPoints()))
      return;

    if (!use_even_odd || num_raster_points > MAX_AIRSPACE_VERTICES) {
      VisitPreparedPolygon(airspace, as_type_or_class);
      return;
    }

    const AirspaceClassRendererSettings &class_settings =
      settings.classes[as_type_or_class];

    bool fill_airspace = warning_manager.HasWarning(airspace) ||
      warning_manager.IsInside(airspace) ||
      class_settings.fill_mode ==
      AirspaceClassRendererSettings::FillMode::ALL;

    if (!warning_manager.IsAcked(airspace) &&
        class_settings.fill_mode !=
        AirspaceClassRendererSettings::FillMode::NONE) {
      const GLEnable<GL_STENCIL_TEST> stencil;

      if (!fill_airspace) {
        // set stencil for filling (bit 0)
        num_padding_vertices = BuildPaddingLoop(raster_points.data(),
                                                num_raster_points,
                                                look.thick_pen.GetWidth());
        if (num_padding_vertices == 0) {
          VisitPreparedPolygon(airspace, as_type_or_class);
          return;
        }
        SetFillStencil();
        DrawPreparedPadding();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }

      // fill interior without overpainting any previous outlines
      {
        SetupInterior(airspace, !fill_airspace);
        const GLEnable<GL_BLEND> blend;
        DrawEvenOddPolygon(canvas, raster_points.data(), num_raster_points,
                           fill_airspace ? FILL_STENCIL : POLYGON_STENCIL,
                           fill_airspace ? 0 : FILL_STENCIL,
                           FILL_STENCIL | OUTLINE_STENCIL);
      }

      if (!fill_airspace) {
        // clear fill stencil (bit 0)
        ClearFillStencil();
        DrawPreparedPadding();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }
    }

    // draw outline
    DrawOutline(airspace);
  }

public:
  void Visit(const AbstractAirspace &airspace) {
    switch (airspace.GetShape()) {
    case AbstractAirspace::Shape::CIRCLE:
      VisitCircle((const AirspaceCircle &)airspace);
      break;

    case AbstractAirspace::Shape::POLYGON:
      VisitPolygon((const AirspacePolygon &)airspace);
      break;
    }
  }

private:
  void VisitPreparedPolygon(const AirspacePolygon &airspace,
                            AirspaceClass as_type_or_class) {
    const AirspaceClassRendererSettings &class_settings =
      settings.classes[as_type_or_class];
    const bool fill_airspace = warning_manager.HasWarning(airspace) ||
      warning_manager.IsInside(airspace) ||
      class_settings.fill_mode == AirspaceClassRendererSettings::FillMode::ALL;

    if (!warning_manager.IsAcked(airspace) &&
        class_settings.fill_mode !=
        AirspaceClassRendererSettings::FillMode::NONE) {
      const GLEnable<GL_STENCIL_TEST> stencil;

      if (!fill_airspace) {
        SetFillStencil();
        DrawPrepared();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }

      {
        SetupInterior(airspace, !fill_airspace);
        const GLEnable<GL_BLEND> blend;
        DrawPrepared();
      }

      if (!fill_airspace) {
        ClearFillStencil();
        DrawPrepared();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }
    }

    DrawOutline(airspace);
  }

  void DrawOutline(const AbstractAirspace &airspace) noexcept {
    if (!SetupOutline(airspace))
      return;

    if (use_outline_stencil) {
      const GLEnable<GL_STENCIL_TEST> stencil;
      DrawPrepared();
    } else
      DrawPrepared();
  }

  void DrawOutline(const AbstractAirspace &airspace, PixelPoint center,
                   unsigned radius) noexcept {
    if (!SetupOutline(airspace))
      return;

    if (use_outline_stencil) {
      const GLEnable<GL_STENCIL_TEST> stencil;
      canvas.DrawCircle(center, radius);
    } else
      canvas.DrawCircle(center, radius);
  }

  bool SetupOutline(const AbstractAirspace &airspace) {
    const auto type_or_class = airspace.GetTypeOrClass();
    AirspaceClass as_type_or_class = settings.classes[type_or_class].display
      ? type_or_class : airspace.GetClass();

    if (settings.black_outline)
      canvas.SelectBlackPen();
    else if (settings.classes[as_type_or_class].border_width == 0)
      // Don't draw outlines if border_width == 0
      return false;
    else
      canvas.Select(look.classes[as_type_or_class].border_pen);

    canvas.SelectHollowBrush();

    if (use_outline_stencil) {
      // Set bit 1 in the stencil buffer where the outline is drawn.
      glStencilFunc(GL_ALWAYS, FILL_STENCIL | OUTLINE_STENCIL,
                    FILL_STENCIL | OUTLINE_STENCIL);
      glStencilMask(OUTLINE_STENCIL);
      glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }

    return true;
  }

  void SetupInterior(const AbstractAirspace &airspace,
                     bool check_fillstencil = false) {
    const auto type_or_class = airspace.GetTypeOrClass();
    AirspaceClass as_type_or_class = settings.classes[type_or_class].display
      ? type_or_class : airspace.GetClass();
    const AirspaceClassLook &class_look = look.classes[as_type_or_class];

    // restrict drawing area and don't paint over previously drawn outlines
    if (check_fillstencil)
      glStencilFunc(GL_EQUAL, FILL_STENCIL,
                    FILL_STENCIL | OUTLINE_STENCIL);
    else
      glStencilFunc(GL_EQUAL, 0, OUTLINE_STENCIL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    canvas.Select(Brush(class_look.fill_color.WithAlpha(90)));
    canvas.SelectNullPen();
  }

  void SetFillStencil() {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, FILL_STENCIL | OUTLINE_STENCIL,
                  FILL_STENCIL | OUTLINE_STENCIL);
    glStencilMask(FILL_STENCIL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    canvas.SelectBlackBrush();
    canvas.SelectNullPen();
  }

  void ClearFillStencil() {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, FILL_STENCIL | OUTLINE_STENCIL,
                  FILL_STENCIL | OUTLINE_STENCIL);
    glStencilMask(FILL_STENCIL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    canvas.SelectBlackBrush();
    canvas.SelectNullPen();
  }

  void DrawPreparedPadding() noexcept {
    canvas.DrawFilledTriangles(airspace_workspace.triangles.data(),
                               num_padding_vertices);
  }
};

class AirspaceFillRenderer final
  : protected MapCanvas
{
  const AirspaceLook &look;
  const AirspaceWarningCopy &warning_manager;
  const AirspaceRendererSettings &settings;
  const bool use_even_odd;
  const bool use_outline_stencil;

public:
  AirspaceFillRenderer(Canvas &_canvas, const WindowProjection &_projection,
                       const AirspaceLook &_look,
                       const AirspaceWarningCopy &_warnings,
                       const AirspaceRendererSettings &_settings,
                       bool _use_even_odd, bool _use_outline_stencil)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(1.1)),
     look(_look), warning_manager(_warnings), settings(_settings),
     use_even_odd(_use_even_odd), use_outline_stencil(_use_outline_stencil)
  {
    if (use_outline_stencil || use_even_odd) {
      glStencilMask(0xff);
      glClear(GL_STENCIL_BUFFER_BIT);
    }

    if (settings.fill_mode != AirspaceRendererSettings::FillMode::NONE)
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

private:
  void VisitCircle(const AirspaceCircle &airspace) {
    auto screen_center =
      projection.GeoToScreen(airspace.GetReferenceLocation());
    unsigned screen_radius =
      projection.GeoToScreenDistance(airspace.GetRadius());

    if (!warning_manager.IsAcked(airspace) && SetupInterior(airspace)) {
      if (use_outline_stencil) {
        const GLEnable<GL_STENCIL_TEST> stencil;
        const GLEnable<GL_BLEND> blend;
        canvas.DrawCircle(screen_center, screen_radius);
      } else {
        const GLEnable<GL_BLEND> blend;
        canvas.DrawCircle(screen_center, screen_radius);
      }
    }

    DrawOutline(airspace, screen_center, screen_radius);
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
    if (!PreparePolygon(airspace.GetPoints()))
      return;

    const bool use_polygon_even_odd =
      use_even_odd && num_raster_points <= MAX_AIRSPACE_VERTICES;

    if (!warning_manager.IsAcked(airspace) && SetupInterior(airspace)) {
      if (use_polygon_even_odd) {
        const GLEnable<GL_STENCIL_TEST> stencil;
        const GLEnable<GL_BLEND> blend;
        DrawEvenOddPolygon(canvas, raster_points.data(), num_raster_points,
                           FILL_STENCIL, 0, OUTLINE_STENCIL);
      } else if (use_outline_stencil) {
        const GLEnable<GL_STENCIL_TEST> stencil;
        const GLEnable<GL_BLEND> blend;
        DrawPrepared();
      } else {
        const GLEnable<GL_BLEND> blend;
        DrawPrepared();
      }
    }

    DrawOutline(airspace);
  }

public:
  void Visit(const AbstractAirspace &airspace) {
    switch (airspace.GetShape()) {
    case AbstractAirspace::Shape::CIRCLE:
      VisitCircle((const AirspaceCircle &)airspace);
      break;

    case AbstractAirspace::Shape::POLYGON:
      VisitPolygon((const AirspacePolygon &)airspace);
      break;
    }
  }

private:
  void DrawOutline(const AbstractAirspace &airspace) noexcept {
    if (!SetupOutline(airspace))
      return;

    if (use_outline_stencil) {
      const GLEnable<GL_STENCIL_TEST> stencil;
      DrawPrepared();
    } else
      DrawPrepared();
  }

  void DrawOutline(const AbstractAirspace &airspace, PixelPoint center,
                   unsigned radius) noexcept {
    if (!SetupOutline(airspace))
      return;

    if (use_outline_stencil) {
      const GLEnable<GL_STENCIL_TEST> stencil;
      canvas.DrawCircle(center, radius);
    } else
      canvas.DrawCircle(center, radius);
  }

  bool SetupOutline(const AbstractAirspace &airspace) {
    const auto type_or_class = airspace.GetTypeOrClass();
    AirspaceClass as_type_or_class = settings.classes[type_or_class].display
      ? type_or_class : airspace.GetClass();

    if (settings.black_outline)
      canvas.SelectBlackPen();
    else if (settings.classes[as_type_or_class].border_width == 0)
      // Don't draw outlines if border_width == 0
      return false;
    else
      canvas.Select(look.classes[as_type_or_class].border_pen);

    canvas.SelectHollowBrush();

    if (use_outline_stencil) {
      glStencilFunc(GL_ALWAYS, FILL_STENCIL | OUTLINE_STENCIL,
                    FILL_STENCIL | OUTLINE_STENCIL);
      glStencilMask(OUTLINE_STENCIL);
      glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }

    return true;
  }

  bool SetupInterior(const AbstractAirspace &airspace) {
    const auto type_or_class = airspace.GetTypeOrClass();
    AirspaceClass as_type_or_class = settings.classes[type_or_class].display
      ? type_or_class : airspace.GetClass();
    if (settings.fill_mode == AirspaceRendererSettings::FillMode::NONE)
      return false;

    const AirspaceClassLook &class_look = look.classes[as_type_or_class];

    if (use_outline_stencil) {
      glStencilFunc(GL_EQUAL, 0, OUTLINE_STENCIL);
      glStencilMask(0);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }

    canvas.Select(Brush(class_look.fill_color.WithAlpha(48)));
    canvas.SelectNullPen();

    return true;
  }
};

void
AirspaceRenderer::DrawInternal(Canvas &canvas,
                               const WindowProjection &projection,
                               const AirspaceRendererSettings &settings,
                               const AirspaceWarningCopy &awc,
                               const AirspacePredicate &visible)
{
  const auto range =
    airspaces->QueryWithinRange(projection.GetGeoScreenCenter(),
                                projection.GetScreenDistanceMeters());

  if (settings.fill_mode == AirspaceRendererSettings::FillMode::NONE) {
    AirspaceFillRenderer renderer(canvas, projection, look, awc, settings,
                                  false, false);
    for (const auto &i : range) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (visible(airspace))
        renderer.Visit(airspace);
    }
    return;
  }

  GLint stencil_bits = 0;
  glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);
  const bool use_even_odd = stencil_bits >= EVEN_ODD_STENCIL_BITS;
  const bool use_outline_stencil = stencil_bits >= 2;

  GLStateGuard state_guard;
  if (settings.fill_mode == AirspaceRendererSettings::FillMode::ALL) {
    AirspaceFillRenderer renderer(canvas, projection, look, awc, settings,
                                  use_even_odd, use_outline_stencil);
    for (const auto &i : range) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (visible(airspace))
        renderer.Visit(airspace);
    }
  } else {
    AirspaceVisitorRenderer renderer(canvas, projection, look, awc, settings,
                                     use_even_odd, use_outline_stencil);
    for (const auto &i : range) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (visible(airspace))
        renderer.Visit(airspace);
    }
  }
}

#endif /* ENABLE_OPENGL */
