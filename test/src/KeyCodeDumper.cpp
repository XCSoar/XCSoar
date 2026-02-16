// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_SCREEN
#define ENABLE_BUTTON_LOOK

#include "Main.hpp"
#include "ui/window/SingleWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Form/Button.hpp"

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
  bool OnMouseDown([[maybe_unused]] PixelPoint p) noexcept override {
    SetFocus();
    return true;
  }

  bool OnKeyDown(unsigned key_code) noexcept override {
    add_event(key_code, true);
    return true;
  }

  bool OnKeyUp(unsigned key_code) noexcept override {
    add_event(key_code, false);
    return true;
  }

  void OnSetFocus() noexcept override {
    PaintWindow::OnSetFocus();
    Invalidate();
  }

  void OnKillFocus() noexcept override {
    PaintWindow::OnKillFocus();
    Invalidate();
  }

  void OnPaint(Canvas &canvas) noexcept override {
    canvas.SelectWhiteBrush();
    if (HasFocus())
      canvas.SelectBlackPen();
    else
      canvas.SelectWhitePen();
    canvas.Clear();

    canvas.SetTextColor(COLOR_BLACK);
    canvas.SetBackgroundTransparent();
    canvas.Select(normal_font);

    unsigned text_height = canvas.CalcTextSize(_T("W")).height;
    for (int i = num_events - 1, y = 4; i >= 0; --i, y += text_height) {
      const struct key_event &event = events[i];
      char buffer[64];
      _stprintf(buffer, _T("key %s = 0x%x"),
                event.down ? _T("down") : _T("up"), event.code);
      canvas.DrawText({4, y}, buffer);
    }
  }
};

class TestWindow final : public UI::SingleWindow {
  KeyCodeDumper key_code_dumper;
  Button close_button;

public:
  using UI::SingleWindow::SingleWindow;

  void Create(PixelSize size) {
    SingleWindow::Create(_T("KeyCodeDumper"), size);

    PixelRect rc = GetClientRect();

    PixelRect d_rc = rc;
    d_rc.bottom = (rc.top + rc.bottom + 1) / 2;
    key_code_dumper.Create(*this, d_rc);

    PixelRect button_rc = rc;
    button_rc.top = (rc.top + rc.bottom + 1) / 2;

    close_button.Create(*this, *button_look,
                        _T("Close"), button_rc,
                        WindowStyle(),
                        [this](){ Close(); });

    key_code_dumper.SetFocus();
  }

protected:
  void OnResize(PixelSize new_size) noexcept override {
    SingleWindow::OnResize(new_size);

    if (key_code_dumper.IsDefined())
      key_code_dumper.Move(PixelRect{PixelSize{new_size.width, (new_size.height + 1) / 2}});

    if (close_button.IsDefined())
      close_button.Move({0, int(new_size.height + 1) / 2, int(new_size.width), int(new_size.height / 2)});
  }
};

static void
Main(UI::Display &display)
{
  TestWindow window{display};
  window.Create({240, 100});
  window.Show();

  window.RunEventLoop();
}
