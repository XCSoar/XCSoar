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
#include "Form/Button.hpp"
#include "Form/ActionListener.hpp"
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
  virtual void OnPaint(Canvas &canvas) override {
    canvas.ClearWhite();
    HorizonRenderer::Draw(canvas, canvas.GetRect(), look, attitude);
  }
};

static void
Main()
{
  HorizonLook horizon_look;
  horizon_look.Initialise();

  WindowStyle with_border;
  with_border.Border();

  HorizonWindow horizon(horizon_look);
  horizon.Create(main_window, main_window.GetClientRect(), with_border);
  main_window.SetFullWindow(horizon);

  auto timer = MakeLambdaTimer([&horizon](){
      AttitudeState attitude;
      attitude.bank_angle_computed = true;
      attitude.pitch_angle_computed = true;
      attitude.bank_angle = Angle::Zero();
      attitude.pitch_angle = Angle::Zero();

      horizon.SetAttitude(attitude);
    });
  timer.Schedule(250);

  main_window.RunEventLoop();

  timer.Cancel();
}
