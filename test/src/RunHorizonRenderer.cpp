// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_MAIN_WINDOW
#define ENABLE_CLOSE_BUTTON

#include "Main.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Form/Button.hpp"
#include "Look/HorizonLook.hpp"
#include "Renderer/HorizonRenderer.hpp"
#include "NMEA/Attitude.hpp"

class HorizonWindow : public PaintWindow
{
  const HorizonLook &look;
  AttitudeState attitude;

public:
  HorizonWindow(const HorizonLook &_look)
    :look(_look) {}

  void SetAttitude(const AttitudeState &_attitude) {
    attitude = _attitude;
    Invalidate();
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    canvas.ClearWhite();
    HorizonRenderer::Draw(canvas, canvas.GetRect(), look, attitude);
  }
};

static void
Main(TestMainWindow &main_window)
{
  HorizonLook horizon_look;
  horizon_look.Initialise();

  WindowStyle with_border;
  with_border.Border();

  HorizonWindow horizon(horizon_look);
  horizon.Create(main_window, main_window.GetClientRect(), with_border);
  main_window.SetFullWindow(horizon);

  UI::PeriodicTimer timer([&horizon](){
    AttitudeState attitude;
    attitude.bank_angle_available = attitude.pitch_angle_available =
      Validity{TimeStamp{std::chrono::steady_clock::now().time_since_epoch()}};
    attitude.bank_angle = Angle::Zero();
    attitude.pitch_angle = Angle::Zero();

    horizon.SetAttitude(attitude);
  });
  timer.Schedule(std::chrono::milliseconds(250));

  main_window.RunEventLoop();
}
