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
#include <cmath>

/**
 * Stencil bits used by the airspace renderers.
 */
static constexpr GLuint FILL_STENCIL = 1;
static constexpr GLuint OUTLINE_STENCIL = 2;
static constexpr GLuint POLYGON_STENCIL = 4;

/**
 * Build a wide closed line with bevel joins as independent triangles.
 *
 * The generic OpenGL line strip builder changes the strip orientation at
 * acute bends.  Rounded screen coordinates can move a bend across that
 * threshold while panning, making sharp padding corners flicker.  Bevel joins
 * have no angle threshold and remain well-defined all the way to a reversal.
 */
static unsigned
BuildBeveledLoop(const BulkPixelPoint *src, unsigned src_size,
                 float line_width,
                 AllocatedArray<FloatPoint2D> &points,
                 AllocatedArray<FloatPoint2D> &normals,
                 AllocatedArray<FloatPoint2D> &triangles) noexcept
{
  if (src_size < 2)
    return 0;

  points.GrowDiscard(src_size);
  unsigned size = 0;
  for (unsigned i = 0; i < src_size; ++i) {
    if (size == 0 ||
        points[size - 1].x != src[i].x || points[size - 1].y != src[i].y)
      points[size++] = {float(src[i].x), float(src[i].y)};
  }

  if (size > 1 && points[0] == points[size - 1])
    --size;

  if (size < 2)
    return 0;

  const float half_width = line_width * 0.5f;
  normals.GrowDiscard(size);

  unsigned first_valid = size;
  for (unsigned i = 0; i < size; ++i) {
    const auto &a = points[i];
    const auto &b = points[(i + 1) % size];
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float length = std::hypot(dx, dy);
    if (length <= 0)
      continue;

    const float scale = half_width / length;
    normals[i] = {-dy * scale, dx * scale};
    if (first_valid == size)
      first_valid = i;
  }

  if (first_valid == size)
    return 0;

  for (unsigned k = 1; k < size; ++k) {
    const unsigned i = (first_valid + k) % size;
    const auto &a = points[i];
    const auto &b = points[(i + 1) % size];
    if (std::hypot(b.x - a.x, b.y - a.y) <= 0)
      normals[i] = normals[(i + size - 1) % size];
  }

  triangles.GrowDiscard(size * 9);
  unsigned n = 0;
  const auto append = [&triangles, &n](FloatPoint2D p) noexcept {
    triangles[n++] = p;
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

    const auto &previous_normal = normals[(i + size - 1) % size];
    const auto bend = CrossProduct(previous_normal, normal);
    if (bend > 0) {
      append(a);
      append(a - previous_normal);
      append(a - normal);
    } else if (bend < 0) {
      append(a);
      append(a + previous_normal);
      append(a + normal);
    }
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
  static AllocatedArray<FloatPoint2D> fan_buffer;
  fan_buffer.GrowDiscard(num_points + 2);
  fan_buffer[0] = {
    (float(min_x) + float(max_x)) * 0.5f + 0.25f,
    (float(min_y) + float(max_y)) * 0.5f + 0.375f,
  };
  for (unsigned i = 0; i < num_points; ++i)
    fan_buffer[i + 1] = {float(points[i].x), float(points[i].y)};
  fan_buffer[num_points + 1] = fan_buffer[1];

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glStencilFunc(GL_ALWAYS, polygon_stencil, polygon_stencil);
  glStencilMask(polygon_stencil);
  glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
  canvas.DrawFilledTriangleFan(fan_buffer.data(), num_points + 2);

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

class AirspaceVisitorRenderer final
  : protected MapCanvas
{
  const AirspaceLook &look;
  const AirspaceWarningCopy &warning_manager;
  const AirspaceRendererSettings &settings;

  AllocatedArray<FloatPoint2D> padding_points;
  AllocatedArray<FloatPoint2D> padding_normals;
  AllocatedArray<FloatPoint2D> padding_triangles;
  unsigned num_padding_vertices = 0;

public:
  AirspaceVisitorRenderer(Canvas &_canvas, const WindowProjection &_projection,
                          const AirspaceLook &_look,
                          const AirspaceWarningCopy &_warnings,
                          const AirspaceRendererSettings &_settings)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(1.1)),
     look(_look), warning_manager(_warnings), settings(_settings)
  {
    glStencilMask(0xff);
    glClear(GL_STENCIL_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  ~AirspaceVisitorRenderer() {
    glStencilMask(0xff);
  }

private:
  void VisitCircle(const AirspaceCircle &airspace) {
	AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();
    const AirspaceClassRendererSettings &class_settings =
      settings.classes[as_type_or_class];
    const AirspaceClassLook &class_look = look.classes[as_type_or_class];

    auto screen_center = projection.GeoToScreen(airspace.GetReferenceLocation());
    unsigned screen_radius = projection.GeoToScreenDistance(airspace.GetRadius());

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
    if (SetupOutline(airspace))
      canvas.DrawCircle(screen_center, screen_radius);
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
	AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();
    if (!PreparePolygon(airspace.GetPoints()))
      return;

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
        num_padding_vertices =
          BuildBeveledLoop(raster_points.data(), num_raster_points,
                           look.thick_pen.GetWidth(), padding_points,
                           padding_normals, padding_triangles);
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
    if (SetupOutline(airspace))
      DrawPrepared();
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
  bool SetupOutline(const AbstractAirspace &airspace) {
    AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();

    if (settings.black_outline)
      canvas.SelectBlackPen();
    else if (settings.classes[as_type_or_class].border_width == 0)
      // Don't draw outlines if border_width == 0
      return false;
    else
      canvas.Select(look.classes[as_type_or_class].border_pen);

    canvas.SelectHollowBrush();

    // set bit 1 in stencil buffer, where an outline is drawn
    glStencilFunc(GL_ALWAYS, FILL_STENCIL | OUTLINE_STENCIL,
                  FILL_STENCIL | OUTLINE_STENCIL);
    glStencilMask(OUTLINE_STENCIL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    return true;
  }

  void SetupInterior(const AbstractAirspace &airspace,
                     bool check_fillstencil = false) {
	AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();
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
    canvas.DrawFilledTriangles(padding_triangles.data(),
                               num_padding_vertices);
  }
};

class AirspaceFillRenderer final
  : protected MapCanvas
{
  const AirspaceLook &look;
  const AirspaceWarningCopy &warning_manager;
  const AirspaceRendererSettings &settings;

public:
  AirspaceFillRenderer(Canvas &_canvas, const WindowProjection &_projection,
                       const AirspaceLook &_look,
                       const AirspaceWarningCopy &_warnings,
                       const AirspaceRendererSettings &_settings)
    :MapCanvas(_canvas, _projection,
               _projection.GetScreenBounds().Scale(1.1)),
     look(_look), warning_manager(_warnings), settings(_settings)
  {
    glStencilMask(0xff);
    glClear(GL_STENCIL_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  ~AirspaceFillRenderer() {
    glStencilMask(0xff);
  }

private:
  void VisitCircle(const AirspaceCircle &airspace) {
    auto screen_center = projection.GeoToScreen(airspace.GetReferenceLocation());
    unsigned screen_radius = projection.GeoToScreenDistance(airspace.GetRadius());

    if (!warning_manager.IsAcked(airspace) && SetupInterior(airspace)) {
      const GLEnable<GL_BLEND> blend;
      canvas.DrawCircle(screen_center, screen_radius);
    }

    // draw outline
    if (SetupOutline(airspace))
      canvas.DrawCircle(screen_center, screen_radius);
  }

  void VisitPolygon(const AirspacePolygon &airspace) {
    if (!PreparePolygon(airspace.GetPoints()))
      return;

    if (!warning_manager.IsAcked(airspace) && SetupInterior(airspace)) {
      // fill interior without overpainting any previous outlines
      const GLEnable<GL_STENCIL_TEST> stencil;
      const GLEnable<GL_BLEND> blend;
      DrawEvenOddPolygon(canvas, raster_points.data(), num_raster_points,
                         FILL_STENCIL, 0, 0);
    }

    // draw outline
    if (SetupOutline(airspace))
      DrawPrepared();
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
  bool SetupOutline(const AbstractAirspace &airspace) {
    AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();

    if (settings.black_outline)
      canvas.SelectBlackPen();
    else if (settings.classes[as_type_or_class].border_width == 0)
      // Don't draw outlines if border_width == 0
      return false;
    else
      canvas.Select(look.classes[as_type_or_class].border_pen);

    canvas.SelectHollowBrush();

    return true;
  }

  bool SetupInterior(const AbstractAirspace &airspace) {
	AirspaceClass as_type_or_class = settings.classes[airspace.GetTypeOrClass()].display ? airspace.GetTypeOrClass() : airspace.GetClass();
    if (settings.fill_mode == AirspaceRendererSettings::FillMode::NONE)
      return false;

    const AirspaceClassLook &class_look = look.classes[as_type_or_class];

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

  if (settings.fill_mode == AirspaceRendererSettings::FillMode::ALL ||
      settings.fill_mode == AirspaceRendererSettings::FillMode::NONE) {
    AirspaceFillRenderer renderer(canvas, projection, look, awc, settings);
    for (const auto &i : range) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (visible(airspace))
        renderer.Visit(airspace);
    }
  } else {
    AirspaceVisitorRenderer renderer(canvas, projection, look, awc, settings);
    for (const auto &i : range) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (visible(airspace))
        renderer.Visit(airspace);
    }
  }
}

#endif /* ENABLE_OPENGL */
