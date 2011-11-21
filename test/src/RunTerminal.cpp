/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Screen/TerminalWindow.hpp"
#include "Screen/Init.hpp"
#include "Screen/Fonts.hpp"
#include "Look/TerminalLook.hpp"

#include <stdlib.h>

class TestWindow : public SingleWindow {
  TerminalWindow terminal;

  timer_t timer;

public:
  TestWindow(const TerminalLook &look)
    :terminal(look) {}

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
    wc.lpszClassName = _T("RunTerminal");

    return RegisterClass(&wc);
  }
#endif /* USE_GDI */

  void set(PixelScalar left, PixelScalar top,
           UPixelScalar width, UPixelScalar height) {
    SingleWindow::set(_T("RunTerminal"), _T("RunTerminal"),
                      left, top, width, height);

    PixelRect rc = get_client_rect();

    terminal.set(*this, rc.left, rc.top,
                 rc.right - rc.left, rc.bottom - rc.top);
  }

protected:
  virtual bool on_create() {
    SingleWindow::on_create();
    timer = set_timer(1, 10);
    return true;
  }

  virtual bool on_destroy() {
    kill_timer(timer);
    SingleWindow::on_destroy();
    return true;
  }

  virtual bool on_timer(timer_t id) {
    if (id == timer) {
      unsigned r = rand();
      char ch;
      if ((r % 16) == 0)
        ch = '\n';
      else
        ch = 0x20 + ((r / 16) % 0x60);
      terminal.Write(&ch, 1);
      return true;
    } else
      return SingleWindow::on_timer(id);
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
  Fonts::Initialize();

#ifdef USE_GDI
  TestWindow::register_class(hInstance);
#endif

  TerminalLook look;
  look.Initialise(Fonts::monospace);

  TestWindow window(look);
  window.set(0, 0, 400, 400);
  window.show();

  window.event_loop();

  Fonts::Deinitialize();

  return 0;
}
