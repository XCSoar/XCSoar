// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_SCREEN
#define ENABLE_BUTTON_LOOK

#if defined(__GNUC__) && !defined(__clang__)
/* this warning is bogus because GCC is not clever enough to
   understand that the switch/case in paint() always initialises the
   "label" variable */
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include "Main.hpp"
#include "Math/Angle.hpp"
#include "ui/window/SingleWindow.hpp"
#include "ui/canvas/BufferCanvas.hpp"
#include "Form/Button.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "util/Compiler.h"

#ifndef ENABLE_OPENGL
#include "ui/canvas/WindowCanvas.hpp"
#endif

#include <algorithm>

class TestWindow final : public UI::SingleWindow {
#ifndef ENABLE_OPENGL
  Button buffer_button;
#endif
  Button close_button;
  unsigned page = 0;
#ifndef ENABLE_OPENGL
  bool buffered = false;
  BufferCanvas buffer;
#endif

  ButtonFrameRenderer button_renderer{*button_look};

public:
  using UI::SingleWindow::SingleWindow;

  void Create(PixelSize size) {
    SingleWindow::Create("RunCanvas", size);

    PixelRect rc = GetClientRect();

    PixelRect button_rc = rc;
    button_rc.bottom -= 5;
    button_rc.top = button_rc.bottom - 25;

#ifndef ENABLE_OPENGL
    button_rc.left += 5;
    button_rc.right = button_rc.left + 65;

    buffer_button.Create(*this, *button_look, "Buffer", button_rc,
                         WindowStyle(),
                         [this](){
                           buffered = !buffered;
                           if (buffered) {
                             WindowCanvas canvas(*this);
                             buffer.Create(canvas, canvas.GetSize());
                           } else
                             buffer.Destroy();
                           update();
                         });
#endif

    button_rc.right = rc.right - 5;
    button_rc.left = button_rc.right - 65;

    close_button.Create(*this, *button_look, "Close", button_rc,
                        WindowStyle(),
                        [this](){ Close(); });
  }

private:
  void paint(Canvas &canvas) {
    canvas.SelectHollowBrush();
    canvas.SelectBlackPen();

    Brush red_brush(COLOR_RED);

    const PixelRect rc = GetClientRect();
    const int width = rc.GetWidth(), height = rc.GetHeight();
    const PixelPoint center = rc.GetCenter();

    BulkPixelPoint p1[3] = {
      { -100, center.y },
      { (width * 2) / 3, -100 },
      { center.x, height * 2 },
    };

    BulkPixelPoint p2[3] = {
      { -2000, center.y },
      { width * 10, -3000 },
      { width * 5, 3000 },
    };

    const char *label;
    switch (page) {
    case 0:
      canvas.DrawSegment(center,
                         std::min(width, height) / 3,
                     Angle::Zero(), Angle::Degrees(90),
                     false);
      label = "segment 0-90 horizon=false";
      break;

    case 1:
      canvas.DrawSegment(center,
                         std::min(width, height) / 3,
                     Angle::Degrees(45), Angle::Degrees(180),
                     true);
      label = "segment 45-180 horizon=true";
      break;

    case 2:
      canvas.DrawCircle(center,
                        std::min(width, height) / 3);
      label = "circle";
      break;

    case 3:
    case 4: {
      PixelRect rc;
      rc.left = center.x - 50;
      rc.top = center.y - 20;
      rc.right = center.x + 50;
      rc.bottom = center.y + 20;
      button_renderer.DrawButton(canvas, rc,
                                 page == 4 ? ButtonState::PRESSED : ButtonState::ENABLED);
      label = page == 4
        ? "button down=true" : "button down=false";
    }
      break;

    case 5:
      canvas.Select(red_brush);
      canvas.DrawPolygon(p1, 3);
      label = "big polygon";
      break;

    case 6:
      canvas.Select(red_brush);
      canvas.DrawPolygon(p2, 3);
      label = "huge polygon";
      break;

    default:
      gcc_unreachable();
    }

    canvas.SetTextColor(Color(0, 0, 128));
    canvas.SetBackgroundTransparent();
    canvas.Select(normal_font);
    canvas.DrawText({5, 5}, label);
#ifndef ENABLE_OPENGL
    canvas.DrawText({5, 25},
                    buffered ? "buffered" : "not buffered");
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
  bool OnMouseDown(PixelPoint p) noexcept override {
    if (SingleWindow::OnMouseDown(p))
      return true;

    page = (page + 1) % 7;
    update();
    return true;
  }

  void OnPaint(Canvas &canvas) noexcept override {
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

static void
Main(UI::Display &display)
{
  TestWindow window{display};
  window.Create({250, 250});
  window.Show();

  window.RunEventLoop();
}
