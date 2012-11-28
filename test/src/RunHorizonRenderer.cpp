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
#include "Screen/Timer.hpp"
#include "Screen/Canvas.hpp"
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
  void OnPaint(Canvas &canvas) {
    canvas.ClearWhite();
    HorizonRenderer::Draw(canvas, canvas.GetRect(), look, attitude);
  }
};

class TestWindow : public SingleWindow
{
  ButtonWindow close_button;
  HorizonWindow horizon;

  WindowTimer timer;

  enum {
    ID_START = 100,
    ID_CLOSE
  };

public:
  TestWindow(const HorizonLook &look):horizon(look), timer(*this)
  {
    timer.Schedule(250);
  }

  ~TestWindow() {
    timer.Cancel();
  }

  void Create(PixelSize size) {
    TopWindowStyle style;
    style.Resizable();

    SingleWindow::Create(_T("RunHorizonRenderer"), size, style);

    const PixelRect rc = GetClientRect();

    WindowStyle with_border;
    with_border.Border();

    horizon.Create(*this, rc, with_border);

    PixelRect button_rc = rc;
    button_rc.top = button_rc.bottom - 30;
    close_button.Create(*this, _T("Close"), ID_CLOSE, button_rc);
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

  virtual bool OnTimer(WindowTimer &_timer) {
    if (_timer == timer) {
      AttitudeState attitude;
      attitude.bank_angle_available = true;
      attitude.pitch_angle_available = true;
      attitude.bank_angle = Angle::Zero();
      attitude.pitch_angle = Angle::Zero();

      horizon.SetAttitude(attitude);
      return true;
    }

    return SingleWindow::OnTimer(_timer);
  }

  virtual void OnResize(UPixelScalar width, UPixelScalar height) {
    SingleWindow::OnResize(width, height);
    horizon.Resize(width, height);
  }
};

static void
Main()
{
  HorizonLook horizon_look;
  horizon_look.Initialise();

  TestWindow window(horizon_look);
  window.Create({160, 160});

  window.Show();
  window.RunEventLoop();
}
