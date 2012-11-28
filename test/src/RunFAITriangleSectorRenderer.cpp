/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#define ENABLE_SCREEN

#include "Main.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Canvas.hpp"
#include "Renderer/FAITriangleAreaRenderer.hpp"
#include "Geo/GeoPoint.hpp"
#include "Projection/WindowProjection.hpp"

class FAITriangleWindow : public PaintWindow
{
protected:
  void OnPaint(Canvas &canvas) {
    canvas.ClearWhite();

    const GeoPoint a(Angle::Degrees(fixed(7.70722)),
                     Angle::Degrees(fixed(51.052)));
    const GeoPoint b(Angle::Degrees(fixed(11.5228)),
                     Angle::Degrees(fixed(50.3972)));

    WindowProjection projection;
    projection.SetScreenOrigin(canvas.GetWidth() / 2, canvas.GetHeight() / 2);
    projection.SetGeoLocation(a.Middle(b));
    projection.SetScreenSize(canvas.GetWidth(), canvas.GetHeight());
    projection.SetScaleFromRadius(fixed(400000));
    projection.UpdateScreenBounds();

    canvas.SelectBlackPen();
    canvas.SelectHollowBrush();

    RasterPoint pa = projection.GeoToScreen(a);
    canvas.DrawCircle(pa.x, pa.y, 4);

    RasterPoint pb = projection.GeoToScreen(b);
    canvas.DrawCircle(pb.x, pb.y, 4);

    RenderFAISector(canvas, projection, a, b, false);
  }
};

class TestWindow : public SingleWindow
{
  ButtonWindow close_button;
  FAITriangleWindow triangle_window;

  enum {
    ID_START = 100,
    ID_CLOSE
  };

public:
  void Create(PixelSize size) {
    TopWindowStyle style;
    style.Resizable();

    SingleWindow::Create(_T("RunFAITriangleSectorRenderer"),
                         size, style);

    const PixelRect rc = GetClientRect();

    WindowStyle with_border;
    with_border.Border();

    PixelRect button_rc = rc;
    button_rc.top = button_rc.bottom - 30;
    close_button.Create(*this, _T("Close"), ID_CLOSE, button_rc);
    close_button.SetFont(normal_font);

    triangle_window.Create(*this, rc, with_border);
  }

protected:
  virtual bool OnCommand(unsigned id, unsigned code) {
    switch (id) {
    case ID_CLOSE:
      Close();
      return true;
    }

    return SingleWindow::OnCommand(id, code);
  }

  virtual void OnResize(UPixelScalar width, UPixelScalar height) {
    SingleWindow::OnResize(width, height);
    triangle_window.Resize(width, height);
  }
};

static void
Main()
{
  TestWindow window;
  window.Create({640, 480});

  window.Show();
  window.RunEventLoop();
}
