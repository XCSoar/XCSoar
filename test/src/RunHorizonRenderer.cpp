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

#include <cmath>

class HorizonWindow : public PaintWindow
{
  const HorizonLook &look;
  AttitudeState attitude;
  HorizonRenderer::HorizonReadout readout;

public:
  HorizonWindow(const HorizonLook &_look)
    :look(_look) {}

  void SetState(const AttitudeState &_attitude,
                const HorizonRenderer::HorizonReadout &_readout) {
    attitude = _attitude;
    readout = _readout;
    Invalidate();
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    canvas.ClearWhite();
    HorizonRenderer::Draw(canvas, canvas.GetRect(), look, attitude,
                          readout);
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

  UI::PeriodicTimer timer([&horizon]() {
    /* Seconds since the demo started.  The timer fires every 50 ms. */
    static double t = 0;
    t += 0.05;

    /* A slow roll into and out of a turn, not a constant-rate sweep. */
    const double bank = 32.0 * std::sin(t * 0.40);
    /* Gentle pitch of its own, plus a little nose-up while banked. */
    const double pitch = 7.0 * std::sin(t * 0.17)
      + 5.0 * (bank * bank) / (32.0 * 32.0);

    AttitudeState attitude;
    attitude.bank_angle_available = attitude.pitch_angle_available = Validity{
        TimeStamp{std::chrono::steady_clock::now().time_since_epoch()}};
    attitude.bank_angle = Angle::Degrees(bank);
    attitude.pitch_angle = Angle::Degrees(pitch);

    /* Already in display units: the demo has no unit settings. */
    HorizonRenderer::HorizonReadout readout;
    readout.speed = 110 + 20 * std::sin(t * 0.23);
    readout.altitude = 1500 + 200 * std::sin(t * 0.11);

    horizon.SetState(attitude, readout);
  });
  timer.Schedule(std::chrono::milliseconds(50));
  main_window.RunEventLoop();
}
