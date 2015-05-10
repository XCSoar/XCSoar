/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

class FAITriangleWindow : public PaintWindow
{
  FAITriangleSettings settings;

public:
  FAITriangleWindow() {
    settings.SetDefaults();
  }

protected:
  virtual void OnPaint(Canvas &canvas) override {
    canvas.ClearWhite();

    const GeoPoint a(Angle::Degrees(7.70722),
                     Angle::Degrees(51.052));
    const GeoPoint b(Angle::Degrees(11.5228),
                     Angle::Degrees(50.3972));

    WindowProjection projection;
    projection.SetScreenOrigin(canvas.GetWidth() / 2, canvas.GetHeight() / 2);
    projection.SetGeoLocation(a.Middle(b));
    projection.SetScreenSize(canvas.GetSize());
    projection.SetScaleFromRadius(fixed(400000));
    projection.UpdateScreenBounds();

    canvas.SelectBlackPen();
    canvas.SelectHollowBrush();

    RasterPoint pa = projection.GeoToScreen(a);
    canvas.DrawCircle(pa.x, pa.y, 4);

    RasterPoint pb = projection.GeoToScreen(b);
    canvas.DrawCircle(pb.x, pb.y, 4);

    RenderFAISector(canvas, projection, a, b, false, settings);
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
