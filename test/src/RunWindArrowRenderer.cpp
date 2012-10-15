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

#include "Screen/SingleWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Init.hpp"
#include "Screen/Timer.hpp"
#include "Fonts.hpp"
#include "Look/DialogLook.hpp"
#include "Look/WindArrowLook.hpp"
#include "Look/Fonts.hpp"
#include "Renderer/WindArrowRenderer.hpp"
#include "Geo/SpeedVector.hpp"

#ifdef USE_GDI
#include "ResourceLoader.hpp"
#endif

class WindWindow : public PaintWindow
{
  WindArrowRenderer renderer;
  SpeedVector wind;

public:
  WindWindow(const WindArrowLook &look)
    :renderer(look), wind(fixed(10), fixed_zero) {}

  SpeedVector GetWind() const {
    return wind;
  }

  void SetWind(const SpeedVector &_wind) {
    wind = _wind;
    Invalidate();
  }

protected:
  void OnPaint(Canvas &canvas) {
    canvas.ClearWhite();

    const PixelRect rc = canvas.GetRect();

    RasterPoint pt = {
      (PixelScalar)(rc.right / 2), (PixelScalar)(rc.bottom / 2)
    };

    canvas.SelectBlackPen();
    canvas.SelectHollowBrush();
    canvas.DrawCircle(pt.x, pt.y, 2);

    renderer.Draw(canvas, Angle::Zero(), wind, pt, rc, WindArrowStyle::ARROW_HEAD);
  }
};

class TestWindow : public SingleWindow
{
  ButtonWindow close_button;
  WindWindow wind;

  WindowTimer timer;

  enum {
    ID_START = 100,
    ID_CLOSE
  };

public:
  TestWindow(const WindArrowLook &look):wind(look), timer(*this)
  {
    timer.Schedule(250);
  }

  ~TestWindow() {
    timer.Cancel();
  }


#ifdef USE_GDI
  static bool register_class(HINSTANCE hInstance) {
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Window::WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = 0;
    wc.lpszClassName = _T("RunWindArrowRenderer");

    return RegisterClass(&wc);
  }
#endif /* USE_GDI */

  void Create(PixelRect _rc) {
    TopWindowStyle style;
    style.Resizable();

    SingleWindow::Create(_T("RunWindArrowRenderer"),
                         _T("RunWindArrowRenderer"),
                         _rc, style);

    const PixelRect rc = GetClientRect();

    WindowStyle with_border;
    with_border.Border();

    wind.Create(*this, rc, with_border);

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
      SpeedVector _wind = wind.GetWind();

      _wind.bearing = (_wind.bearing + Angle::Degrees(fixed(5))).AsBearing();
      _wind.norm += fixed_one;
      if (_wind.norm > fixed(15))
        _wind.norm = fixed_zero;

      wind.SetWind(_wind);
      return true;
    }

    return SingleWindow::OnTimer(_timer);
  }

  virtual void OnResize(UPixelScalar width, UPixelScalar height) {
    SingleWindow::OnResize(width, height);
    wind.Resize(width, height);
  }
};

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#ifdef _WIN32_WCE
        LPWSTR lpCmdLine,
#else
        LPSTR lpCmdLine2,
#endif
        int nCmdShow)
#endif
{
  ScreenGlobalInit screen_init;

#ifdef USE_GDI
  ResourceLoader::Init(hInstance);
  TestWindow::register_class(hInstance);
#endif

  Fonts::Initialize();

  WindArrowLook wind_look;
  wind_look.Initialise();

  TestWindow window(wind_look);
  window.Create(PixelRect{0, 0, 160, 160});

  window.Show();
  window.RunEventLoop();

  Fonts::Deinitialize();

  return 0;
}
