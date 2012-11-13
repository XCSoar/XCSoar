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
#include "Screen/BufferCanvas.hpp"
#include "Screen/Init.hpp"

#ifndef ENABLE_OPENGL
#include "Screen/WindowCanvas.hpp"
#endif

#ifndef _MSC_VER
#include <algorithm>
using std::min;
#endif

class TestWindow : public SingleWindow {
#ifndef ENABLE_OPENGL
  ButtonWindow buffer_button;
#endif
  ButtonWindow close_button;
  unsigned page;
#ifndef ENABLE_OPENGL
  bool buffered;
  BufferCanvas buffer;
#endif

  enum {
    ID_START = 100,
#ifndef ENABLE_OPENGL
    ID_BUFFER,
#endif
    ID_CLOSE
  };

public:
  TestWindow():page(0)
#ifndef ENABLE_OPENGL
              , buffered(false)
#endif
  {}

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
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = _T("RunCanvas");

    return RegisterClass(&wc);
  }
#endif /* USE_GDI */

  void Create(PixelRect _rc) {
    SingleWindow::Create(_T("RunCanvas"), _T("RunCanvas"), _rc);

    PixelRect rc = GetClientRect();

    PixelRect button_rc = rc;
    button_rc.bottom -= 5;
    button_rc.top = button_rc.bottom - 25;

#ifndef ENABLE_OPENGL
    button_rc.left += 5;
    button_rc.right = button_rc.left + 65;

    buffer_button.Create(*this, _T("Buffer"), ID_BUFFER, button_rc);
#endif

    button_rc.right = rc.right - 5;
    button_rc.left = button_rc.right - 65;

    close_button.Create(*this, _T("Close"), ID_CLOSE, button_rc);
  }

private:
  void paint(Canvas &canvas) {
    canvas.SelectHollowBrush();
    canvas.SelectBlackPen();

    Brush red_brush(COLOR_RED);

    const PixelRect rc = GetClientRect();
    const UPixelScalar width = rc.right - rc.left;
    const UPixelScalar height = rc.bottom - rc.top;
    const UPixelScalar hmiddle = (rc.left + rc.right) / 2;
    const UPixelScalar vmiddle = (rc.top + rc.bottom) / 2;

    RasterPoint p1[3] = {
      { -100, PixelScalar(vmiddle) },
      { PixelScalar((width * 2) / 3), -100 },
      { PixelScalar(hmiddle), PixelScalar(height * 2) },
    };

    RasterPoint p2[3] = {
      { -2000, PixelScalar(vmiddle) },
      { PixelScalar(width * 10), -3000 },
      { PixelScalar(width * 5), 3000 },
    };

    const TCHAR *label;
    switch (page) {
    case 0:
      canvas.DrawSegment(hmiddle, vmiddle,
                     min(width, height) / 3,
                     Angle::Degrees(fixed_zero), Angle::Degrees(fixed(90)),
                     false);
      label = _T("segment 0-90 horizon=false");
      break;

    case 1:
      canvas.DrawSegment(hmiddle, vmiddle,
                     min(width, height) / 3,
                     Angle::Degrees(fixed(45)), Angle::Degrees(fixed_180),
                     true);
      label = _T("segment 45-180 horizon=true");
      break;

    case 2:
      canvas.DrawCircle(hmiddle, vmiddle,
                    min(width, height) / 3);
      label = _T("circle");
      break;

    case 3:
    case 4:
      PixelRect rc;
      rc.left = hmiddle - 50;
      rc.top = vmiddle - 20;
      rc.right = hmiddle + 50;
      rc.bottom = vmiddle + 20;
      canvas.DrawButton(rc, page == 4);
      label = page == 4
        ? _T("button down=true") : _T("button down=false");
      break;

    case 5:
      canvas.Select(red_brush);
      canvas.DrawPolygon(p1, 3);
      label = _T("big polygon");
      break;

    case 6:
      canvas.Select(red_brush);
      canvas.DrawPolygon(p2, 3);
      label = _T("huge polygon");
      break;
    }

    canvas.SetTextColor(Color(0, 0, 128));
    canvas.DrawText(5, 5, label);
#ifndef ENABLE_OPENGL
    canvas.DrawText(5, 25,
                    buffered ? _T("buffered") : _T("not buffered"));
#endif
  }

  void update() {
#ifndef ENABLE_OPENGL
    if (buffered) {
      buffer.ClearWhite();

      paint(buffer);
    }
#endif

    Invalidate();
  }

protected:
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) {
    if (SingleWindow::OnMouseDown(x, y))
      return true;

    page = (page + 1) % 7;
    update();
    return true;
  }

  virtual bool OnCommand(unsigned id, unsigned code) {
    switch (id) {
    case ID_CLOSE:
      Close();
      return true;

#ifndef ENABLE_OPENGL
    case ID_BUFFER:
      buffered = !buffered;
      if (buffered) {
        WindowCanvas canvas(*this);
        buffer.Create(canvas, canvas.GetWidth(), canvas.GetHeight());
      } else
        buffer.Destroy();
      update();
      return true;
#endif
    }

    return SingleWindow::OnCommand(id, code);
  }

  virtual void OnPaint(Canvas &canvas) {
#ifndef ENABLE_OPENGL
    if (!buffered) {
#endif
      canvas.ClearWhite();

      paint(canvas);
#ifndef ENABLE_OPENGL
    } else
      canvas.Copy(buffer);
#endif

    SingleWindow::OnPaint(canvas);
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
  TestWindow::register_class(hInstance);
#endif

  TestWindow window;
  window.Create(PixelRect{0, 0, 250, 250});
  window.Show();

  window.RunEventLoop();

  return 0;
}
