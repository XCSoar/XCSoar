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

  double bank_angle = 0;
  double pitch_angle = 0;

  UI::PeriodicTimer timer([&horizon, &pitch_angle, &bank_angle]() {
    // Static variables to keep track of the pitch angle and bank angle
    static double pitch_increment = 1.0;
    static double bank_increment = 1.2;
    static double min_angle = -30.0;
    static double max_angle = 30.0;

    AttitudeState attitude;
    attitude.bank_angle_available = attitude.pitch_angle_available = Validity{
        TimeStamp{std::chrono::steady_clock::now().time_since_epoch()}};

    // Update pitch angle
    if (pitch_angle == max_angle) {
      pitch_increment = -1.0; // Change direction
    } else if (pitch_angle == min_angle) {
      pitch_increment = -pitch_increment; // Reverse direction
    }
    pitch_angle += pitch_increment;

    // Ensure pitch angle stays within bounds [-30, 30]
    if (pitch_angle > max_angle) {
      pitch_angle = max_angle;
    } else if (pitch_angle < min_angle) {
      pitch_angle = min_angle;
    }

    // Update bank angle
    if (bank_angle == max_angle) {
      bank_increment = -1.0; // Change direction
    } else if (bank_angle == min_angle) {
      bank_increment = -bank_increment; // Reverse direction
    }
    bank_angle += bank_increment;

    // Ensure bank angle stays within bounds [-30, 30]
    if (bank_angle > max_angle) {
      bank_angle = max_angle;
    } else if (bank_angle < min_angle) {
      bank_angle = min_angle;
    }

    attitude.bank_angle = Angle::Degrees(bank_angle);
    attitude.pitch_angle = Angle::Degrees(pitch_angle);

    horizon.SetAttitude(attitude);
  });
  timer.Schedule(std::chrono::milliseconds(50));
  main_window.RunEventLoop();
}
