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
#include "Screen/ButtonWindow.hpp"
#include "Screen/Init.hpp"

#include <algorithm>

#include <stdio.h>

class KeyCodeDumper : public PaintWindow {
protected:
  struct key_event {
    unsigned code;
    bool down;
  };

  enum {
    MAX_EVENTS = 8,
  };

protected:
  struct key_event events[MAX_EVENTS];
  unsigned num_events;

public:
  KeyCodeDumper():num_events(0) {}

protected:
  void add_event(unsigned key_code, bool down) {
    if (num_events >= MAX_EVENTS) {
      std::copy(events + 1, events + MAX_EVENTS, events);
      --num_events;
    }

    events[num_events].code = key_code;
    events[num_events].down = down;
    ++num_events;

    invalidate();
  }

protected:
  virtual bool on_mouse_down(int x, int y) {
    set_focus();
    return true;
  }

  virtual bool on_key_down(unsigned key_code) {
    add_event(key_code, true);
    return true;
  }

  virtual bool on_key_up(unsigned key_code) {
    add_event(key_code, false);
    return true;
  }

  virtual bool on_setfocus() {
    PaintWindow::on_setfocus();
    invalidate();
    return true;
  }

  virtual bool on_killfocus() {
    PaintWindow::on_killfocus();
    invalidate();
    return true;
  }

  virtual void on_paint(Canvas &canvas) {
    canvas.white_brush();
    if (has_focus())
      canvas.black_pen();
    else
      canvas.white_pen();
    canvas.clear();

    unsigned text_height = canvas.text_size(_T("W")).cy;
    for (int i = num_events - 1, y = 4; i >= 0; --i, y += text_height) {
      const struct key_event &event = events[i];
      TCHAR buffer[64];
      _stprintf(buffer, _T("key %s = 0x%x"),
                event.down ? _T("down") : _T("up"), event.code);
      canvas.text(4, y, buffer);
    }
  }
};

class TestWindow : public SingleWindow {
  KeyCodeDumper key_code_dumper;
  ButtonWindow close_button;

  enum {
    ID_START = 100,
    ID_CLOSE
  };

public:
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
    wc.lpszClassName = _T("KeyCodeDumper");

    return RegisterClass(&wc);
  }
#endif /* USE_GDI */

  void set(int left, int top, unsigned width, unsigned height) {
    SingleWindow::set(_T("KeyCodeDumper"), _T("KeyCodeDumper"),
                      left, top, width, height);

    PixelRect rc = get_client_rect();

    key_code_dumper.set(*this, rc.left, rc.top,
                        rc.right - rc.left, (rc.bottom - rc.top + 1) / 2);
    close_button.set(*this, _T("Close"), ID_CLOSE,
                     rc.left, (rc.top + rc.bottom + 1) / 2,
                     rc.right - rc.left,
                     (rc.top + rc.bottom) / 2);

    key_code_dumper.set_focus();
  }

protected:
  virtual bool on_resize(unsigned width, unsigned height) {
    SingleWindow::on_resize(width, height);
    key_code_dumper.move(0, 0, width, (height + 1) / 2);
    close_button.move(0, (height + 1) / 2, width, height / 2);
    return true;
  }

  virtual bool on_command(unsigned id, unsigned code) {
    switch (id) {
    case ID_CLOSE:
      close();
      return true;
    }

    return SingleWindow::on_command(id, code);
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
  window.set(0, 0, 240, 100);
  window.show();

  window.event_loop();

  return 0;
}
