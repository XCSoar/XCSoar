/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

    Invalidate();
  }

protected:
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) {
    SetFocus();
    return true;
  }

  virtual bool OnKeyDown(unsigned key_code) {
    add_event(key_code, true);
    return true;
  }

  virtual bool OnKeyUp(unsigned key_code) {
    add_event(key_code, false);
    return true;
  }

  virtual void OnSetFocus() {
    PaintWindow::OnSetFocus();
    Invalidate();
  }

  virtual void OnKillFocus() {
    PaintWindow::OnKillFocus();
    Invalidate();
  }

  virtual void OnPaint(Canvas &canvas) {
    canvas.SelectWhiteBrush();
    if (HasFocus())
      canvas.SelectBlackPen();
    else
      canvas.SelectWhitePen();
    canvas.Clear();

    unsigned text_height = canvas.CalcTextSize(_T("W")).cy;
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

  void set(PixelRect _rc) {
    SingleWindow::set(_T("KeyCodeDumper"), _T("KeyCodeDumper"), _rc);

    PixelRect rc = GetClientRect();

    PixelRect d_rc = rc;
    d_rc.bottom = (rc.top + rc.bottom + 1) / 2;
    key_code_dumper.set(*this, d_rc);

    PixelRect button_rc = rc;
    button_rc.top = (rc.top + rc.bottom + 1) / 2;

    close_button.set(*this, _T("Close"), ID_CLOSE, button_rc);

    key_code_dumper.SetFocus();
  }

protected:
  virtual void OnResize(UPixelScalar width, UPixelScalar height) {
    SingleWindow::OnResize(width, height);
    key_code_dumper.Move(0, 0, width, (height + 1) / 2);
    close_button.Move(0, (height + 1) / 2, width, height / 2);
  }

  virtual bool OnCommand(unsigned id, unsigned code) {
    switch (id) {
    case ID_CLOSE:
      Close();
      return true;
    }

    return SingleWindow::OnCommand(id, code);
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
  window.set(PixelRect{0, 0, 240, 100});
  window.Show();

  window.RunEventLoop();

  return 0;
}
