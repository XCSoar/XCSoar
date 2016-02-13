/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#define ENABLE_MAIN_WINDOW
#define ENABLE_CLOSE_BUTTON

#include "Main.hpp"
#include "Screen/Canvas.hpp"
#include "Form/Button.hpp"
#include "Form/ActionListener.hpp"
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
    PixelPoint p;
    if (projection.GeoToScreenIfVisible(*i, p))
      canvas.DrawCircle(p.x, p.y, 2);
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
  void OnResize(PixelSize new_size) override {
    projection.SetScreenOrigin(new_size.cx / 2, new_size.cy / 2);
    projection.SetGeoLocation(a.Middle(b));
    projection.SetScreenSize(new_size);
    projection.SetScaleFromRadius(400000);
    projection.UpdateScreenBounds();
  }

  bool OnMouseDown(PixelPoint p) override {
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

  bool OnMouseUp(PixelPoint p) override {
    if (drag_mode != DragMode::NONE) {
      drag_mode = DragMode::NONE;
      ReleaseCapture();
      return true;
    }

    return false;
  }

  bool OnMouseMove(PixelPoint p, unsigned keys) override {
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

  virtual void OnPaint(Canvas &canvas) override {
    canvas.ClearWhite();

    canvas.SelectBlackPen();
    canvas.SelectHollowBrush();

    auto pa = projection.GeoToScreen(a);
    canvas.DrawCircle(pa.x, pa.y, 4);

    auto pb = projection.GeoToScreen(b);
    canvas.DrawCircle(pb.x, pb.y, 4);

    RenderFAISector(canvas, projection, a, b, false, settings);
    RenderFAISectorDots(canvas, projection, a, b, false, settings);
  }
};

static void
Main()
{
  FAITriangleWindow triangle_window;

  WindowStyle with_border;
  with_border.Border();

  triangle_window.Create(main_window, main_window.GetClientRect(),
                         with_border);
  main_window.SetFullWindow(triangle_window);

  main_window.RunEventLoop();
}
