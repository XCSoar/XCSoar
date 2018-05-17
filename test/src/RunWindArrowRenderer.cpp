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
#include "Event/LambdaTimer.hpp"
#include "Screen/Canvas.hpp"
#include "Look/WindArrowLook.hpp"
#include "Renderer/WindArrowRenderer.hpp"
#include "Geo/SpeedVector.hpp"

class WindWindow : public PaintWindow
{
  WindArrowRenderer renderer;
  SpeedVector wind;

public:
  WindWindow(const WindArrowLook &look)
    :renderer(look), wind(10, 0) {}

  SpeedVector GetWind() const {
    return wind;
  }

  void SetWind(const SpeedVector &_wind) {
    wind = _wind;
    Invalidate();
  }

protected:
  virtual void OnPaint(Canvas &canvas) override {
    canvas.ClearWhite();

    const PixelRect rc = canvas.GetRect();
    PixelPoint pt = rc.GetCenter();

    canvas.SelectBlackPen();
    canvas.SelectHollowBrush();
    canvas.DrawCircle(pt.x, pt.y, 2);

    renderer.Draw(canvas, Angle::Zero(), wind, pt, rc, WindArrowStyle::ARROW_HEAD);
  }
};

static void
Main()
{
  WindArrowLook wind_look;
  wind_look.Initialise(bold_font);

  WindowStyle with_border;
  with_border.Border();

  WindWindow wind(wind_look);
  wind.Create(main_window, main_window.GetClientRect(), with_border);
  main_window.SetFullWindow(wind);

  auto timer = MakeLambdaTimer([&wind](){
      SpeedVector _wind = wind.GetWind();

      _wind.bearing = (_wind.bearing + Angle::Degrees(5)).AsBearing();
      _wind.norm += 1;
      if (_wind.norm > 15)
        _wind.norm = 0;

      wind.SetWind(_wind);
    });
  timer.Schedule(250);

  main_window.RunEventLoop();

  timer.Cancel();
}
