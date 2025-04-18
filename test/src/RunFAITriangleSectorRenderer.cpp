// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_MAIN_WINDOW
#define ENABLE_CLOSE_BUTTON

#include "Main.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Form/Button.hpp"
#include "Renderer/FAITriangleAreaRenderer.hpp"
#include "Geo/GeoPoint.hpp"
#include "Projection/WindowProjection.hpp"
#include "Engine/Task/Shapes/FAITriangleSettings.hpp"
#include "Engine/Task/Shapes/FAITriangleArea.hpp"

static void
RenderFAISectorDots(Canvas &canvas, const WindowProjection &projection,
                    const GeoPoint &pt1, const GeoPoint &pt2,
                    bool reverse, const FAITriangleSettings &settings)
{
  GeoPoint geo_points[FAI_TRIANGLE_SECTOR_MAX];
  GeoPoint *geo_end = GenerateFAITriangleArea(geo_points, pt1, pt2,
                                              reverse, settings);

  canvas.SelectBlackPen();
  canvas.SelectHollowBrush();

  for (auto *i = geo_points; i != geo_end; ++i) {
    if (auto p = projection.GeoToScreenIfVisible(*i))
      canvas.DrawCircle(*p, 2);
  }
}

class FAITriangleWindow : public PaintWindow
{
  FAITriangleSettings settings;

  GeoPoint a, b;

  WindowProjection projection;

  enum class DragMode {
    NONE,
    A,
    B,
  } drag_mode;

public:
  FAITriangleWindow()
    :a(Angle::Degrees(7.70722), Angle::Degrees(51.052)),
     b(Angle::Degrees(11.5228), Angle::Degrees(50.3972)),
     drag_mode(DragMode::NONE) {
    settings.SetDefaults();
  }

protected:
  void OnResize(PixelSize new_size) noexcept override {
    projection.SetScreenOrigin(PixelRect{new_size}.GetCenter());
    projection.SetGeoLocation(a.Middle(b));
    projection.SetScreenSize(new_size);
    projection.SetScaleFromRadius(400000);
    projection.UpdateScreenBounds();
  }

  bool OnMouseDown(PixelPoint p) noexcept override {
    if (drag_mode != DragMode::NONE)
      return false;

    const GeoPoint gp = projection.ScreenToGeo(p);

    if (projection.GeoToScreenDistance(gp.Distance(a)) < Layout::GetHitRadius()) {
      drag_mode = DragMode::A;
      SetCapture();
      return true;
    }

    if (projection.GeoToScreenDistance(gp.Distance(b)) < Layout::GetHitRadius()) {
      drag_mode = DragMode::B;
      SetCapture();
      return true;
    }

    return false;
  }

  bool OnMouseUp([[maybe_unused]] PixelPoint p) noexcept override {
    if (drag_mode != DragMode::NONE) {
      drag_mode = DragMode::NONE;
      ReleaseCapture();
      return true;
    }

    return false;
  }

  bool OnMouseMove(PixelPoint p, [[maybe_unused]] unsigned keys) noexcept override {
    const GeoPoint gp = projection.ScreenToGeo(p);
    switch (drag_mode) {
    case DragMode::NONE:
      return false;

    case DragMode::A:
      a = gp;
      Invalidate();
      return true;

    case DragMode::B:
      b = gp;
      Invalidate();
      return true;
    }

    gcc_unreachable();
  }

  void OnPaint(Canvas &canvas) noexcept override {
    canvas.ClearWhite();

    canvas.SelectBlackPen();
    canvas.SelectHollowBrush();

    auto pa = projection.GeoToScreen(a);
    canvas.DrawCircle(pa, 4);

    auto pb = projection.GeoToScreen(b);
    canvas.DrawCircle(pb, 4);

    RenderFAISector(canvas, projection, a, b, false, settings);
    RenderFAISectorDots(canvas, projection, a, b, false, settings);
  }
};

static void
Main(TestMainWindow &main_window)
{
  FAITriangleWindow triangle_window;

  WindowStyle with_border;
  with_border.Border();

  triangle_window.Create(main_window, main_window.GetClientRect(),
                         with_border);
  main_window.SetFullWindow(triangle_window);

  main_window.RunEventLoop();
}
