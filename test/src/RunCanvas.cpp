/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

  static bool register_class(HINSTANCE hInstance) {
#ifdef ENABLE_SDL
    return true;
#else /* !ENABLE_SDL */
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
#endif /* !ENABLE_SDL */
  }

  void set(int left, int top, unsigned width, unsigned height) {
    SingleWindow::set(_T("RunCanvas"), _T("RunCanvas"),
                      left, top, width, height);

    PixelRect rc = get_client_rect();

#ifndef ENABLE_OPENGL
    buffer_button.set(*this, _T("Buffer"), ID_BUFFER, 5, rc.bottom - 30, 65, 25);
#endif

    close_button.set(*this, _T("Close"), ID_CLOSE, rc.right - 70, rc.bottom - 30, 65, 25);
  }

private:
  void paint(Canvas &canvas) {
    canvas.hollow_brush();
    canvas.black_pen();

    Brush red_brush(Color::RED);

    RasterPoint p1[3] = {
      { -100, get_vmiddle() },
      { (get_width() * 2) / 3, -100 },
      { get_hmiddle(), get_height() * 2 },
    };

    RasterPoint p2[3] = {
      { -2000, get_vmiddle() },
      { get_width() * 10, -3000 },
      { get_width() * 5, 3000 },
    };

    const TCHAR *label;
    switch (page) {
    case 0:
      canvas.segment(get_hmiddle(), get_vmiddle(),
                     min(get_width(), get_height()) / 3,
                     Angle::degrees(fixed_zero), Angle::degrees(fixed(90)),
                     false);
      label = _T("segment 0-90 horizon=false");
      break;

    case 1:
      canvas.segment(get_hmiddle(), get_vmiddle(),
                     min(get_width(), get_height()) / 3,
                     Angle::degrees(fixed(45)), Angle::degrees(fixed_180),
                     true);
      label = _T("segment 45-180 horizon=true");
      break;

    case 2:
      canvas.circle(get_hmiddle(), get_vmiddle(),
                    min(get_width(), get_height()) / 3);
      label = _T("circle");
      break;

    case 3:
    case 4:
      PixelRect rc;
      rc.left = get_hmiddle() - 50;
      rc.top = get_vmiddle() - 20;
      rc.right = get_hmiddle() + 50;
      rc.bottom = get_vmiddle() + 20;
      canvas.draw_button(rc, page == 4);
      label = page == 4
        ? _T("button down=true") : _T("button down=false");
      break;

    case 5:
      canvas.select(red_brush);
      canvas.polygon(p1, 3);
      label = _T("big polygon");
      break;

    case 6:
      canvas.select(red_brush);
      canvas.polygon(p2, 3);
      label = _T("huge polygon");
      break;
    }

    canvas.set_text_color(Color(0, 0, 128));
    canvas.text(5, 5, label);
#ifndef ENABLE_OPENGL
    canvas.text(5, 25,
                buffered ? _T("buffered") : _T("not buffered"));
#endif
  }

  void update() {
#ifndef ENABLE_OPENGL
    if (buffered) {
      buffer.clear_white();

      paint(buffer);
    }
#endif

    invalidate();
  }

protected:
  virtual bool on_mouse_down(int x, int y) {
    if (SingleWindow::on_mouse_down(x, y))
      return true;

    page = (page + 1) % 7;
    update();
    return true;
  }

  virtual bool on_command(unsigned id, unsigned code) {
    switch (id) {
    case ID_CLOSE:
      close();
      return true;

#ifndef ENABLE_OPENGL
    case ID_BUFFER:
      buffered = !buffered;
      if (buffered) {
        WindowCanvas canvas(*this);
        buffer.set(canvas, canvas.get_width(), canvas.get_height());
      } else
        buffer.reset();
      update();
      return true;
#endif
    }

    return SingleWindow::on_command(id, code);
  }

  /*
  virtual bool on_erase(Canvas &canvas) {
    canvas.clear_white();
    return true;
  }
  */

  virtual void on_paint(Canvas &canvas) {
#ifndef ENABLE_OPENGL
    if (!buffered) {
#endif
      // due to a limitation of our PaintCanvas class, we cannot rely on
      // background erasing with on_erase()
      canvas.clear_white();

      paint(canvas);
#ifndef ENABLE_OPENGL
    } else
      canvas.copy(buffer);
#endif

    SingleWindow::on_paint(canvas);
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

#ifdef WIN32
  TestWindow::register_class(hInstance);
#endif

  TestWindow window;
  window.set(0, 0, 250, 250);
  window.show();

  window.event_loop();

  return 0;
}
