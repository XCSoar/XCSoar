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

#define ENABLE_SCREEN

#include "Main.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Canvas.hpp"

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
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) override {
    SetFocus();
    return true;
  }

  virtual bool OnKeyDown(unsigned key_code) override {
    add_event(key_code, true);
    return true;
  }

  virtual bool OnKeyUp(unsigned key_code) override {
    add_event(key_code, false);
    return true;
  }

  virtual void OnSetFocus() override {
    PaintWindow::OnSetFocus();
    Invalidate();
  }

  virtual void OnKillFocus() override {
    PaintWindow::OnKillFocus();
    Invalidate();
  }

  virtual void OnPaint(Canvas &canvas) override {
    canvas.SelectWhiteBrush();
    if (HasFocus())
      canvas.SelectBlackPen();
    else
      canvas.SelectWhitePen();
    canvas.Clear();

    canvas.SetTextColor(COLOR_BLACK);
    canvas.SetBackgroundTransparent();
    canvas.Select(normal_font);

    unsigned text_height = canvas.CalcTextSize(_T("W")).cy;
    for (int i = num_events - 1, y = 4; i >= 0; --i, y += text_height) {
      const struct key_event &event = events[i];
      TCHAR buffer[64];
      _stprintf(buffer, _T("key %s = 0x%x"),
                event.down ? _T("down") : _T("up"), event.code);
      canvas.DrawText(4, y, buffer);
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
  void Create(PixelSize size) {
    SingleWindow::Create(_T("KeyCodeDumper"), size);

    PixelRect rc = GetClientRect();

    PixelRect d_rc = rc;
    d_rc.bottom = (rc.top + rc.bottom + 1) / 2;
    key_code_dumper.Create(*this, d_rc);

    PixelRect button_rc = rc;
    button_rc.top = (rc.top + rc.bottom + 1) / 2;

    close_button.Create(*this, _T("Close"), ID_CLOSE, button_rc);
    close_button.SetFont(normal_font);

    key_code_dumper.SetFocus();
  }

protected:
  virtual void OnResize(PixelSize new_size) override {
    SingleWindow::OnResize(new_size);

    if (key_code_dumper.IsDefined())
      key_code_dumper.Move(0, 0, new_size.cx, (new_size.cy + 1) / 2);

    if (close_button.IsDefined())
      close_button.Move(0, (new_size.cy + 1) / 2, new_size.cx, new_size.cy / 2);
  }

  virtual bool OnCommand(unsigned id, unsigned code) override {
    switch (id) {
    case ID_CLOSE:
      Close();
      return true;
    }

    return SingleWindow::OnCommand(id, code);
  }
};

static void
Main()
{
  TestWindow window;
  window.Create({240, 100});
  window.Show();

  window.RunEventLoop();
}
