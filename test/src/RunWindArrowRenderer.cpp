// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_MAIN_WINDOW
#define ENABLE_CLOSE_BUTTON

#include "Main.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/WindArrowLook.hpp"
#include "Renderer/WindArrowRenderer.hpp"
#include "Geo/SpeedVector.hpp"
#include "MapSettings.hpp"

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
  void OnPaint(Canvas &canvas) noexcept override {
    canvas.ClearWhite();

    const PixelRect rc = canvas.GetRect();
    PixelPoint pt = rc.GetCenter();

    canvas.SelectBlackPen();
    canvas.SelectHollowBrush();
    canvas.DrawCircle(pt, 2);
    Brush brush;

    renderer.Draw(canvas, Angle::Zero(), wind, pt, rc, WindArrowStyle::ARROW_HEAD, brush);
  }
};

static void
Main(TestMainWindow &main_window)
{
  WindArrowLook wind_look;
  wind_look.Initialise(bold_font);

  WindowStyle with_border;
  with_border.Border();

  WindWindow wind(wind_look);
  wind.Create(main_window, main_window.GetClientRect(), with_border);
  main_window.SetFullWindow(wind);

  UI::PeriodicTimer timer([&wind](){
    SpeedVector _wind = wind.GetWind();

    _wind.bearing = (_wind.bearing + Angle::Degrees(5)).AsBearing();
    _wind.norm += 1;
    if (_wind.norm > 15)
      _wind.norm = 0;

    wind.SetWind(_wind);
  });
  timer.Schedule(std::chrono::milliseconds(250));

  main_window.RunEventLoop();
}
