/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#define ENABLE_BUTTON_LOOK

#include "Main.hpp"
#include "Math/Angle.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Form/Button.hpp"
#include "Form/ActionListener.hpp"
#include "Renderer/ButtonRenderer.hpp"

#ifndef ENABLE_OPENGL
#include "Screen/WindowCanvas.hpp"
#endif

#include <algorithm>

class TestWindow final : public SingleWindow, ActionListener {
#ifndef ENABLE_OPENGL
  Button buffer_button;
#endif
  Button close_button;
  unsigned page;
#ifndef ENABLE_OPENGL
  bool buffered;
  BufferCanvas buffer;
#endif

  ButtonFrameRenderer button_renderer;

  enum Buttons {
#ifndef ENABLE_OPENGL
    BUFFER,
#endif
    CLOSE
  };

public:
  TestWindow():page(0)
#ifndef ENABLE_OPENGL
              , buffered(false)
#endif
              , button_renderer(*button_look)
  {}

  void Create(PixelSize size) {
    SingleWindow::Create(_T("RunCanvas"), size);

    PixelRect rc = GetClientRect();

    PixelRect button_rc = rc;
    button_rc.bottom -= 5;
    button_rc.top = button_rc.bottom - 25;

#ifndef ENABLE_OPENGL
    button_rc.left += 5;
    button_rc.right = button_rc.left + 65;

    buffer_button.Create(*this, *button_look, _T("Buffer"), button_rc,
                        WindowStyle(),
                         *this, BUFFER);
#endif

    button_rc.right = rc.right - 5;
    button_rc.left = button_rc.right - 65;

    close_button.Create(*this, *button_look, _T("Close"), button_rc,
                        WindowStyle(),
                        *this, CLOSE);
  }

private:
  void paint(Canvas &canvas) {
    canvas.SelectHollowBrush();
    canvas.SelectBlackPen();

    Brush red_brush(COLOR_RED);

    const PixelRect rc = GetClientRect();
    const int width = rc.GetWidth(), height = rc.GetHeight();
    const int hmiddle = (rc.left + rc.right) / 2;
    const int vmiddle = (rc.top + rc.bottom) / 2;
    const PixelPoint center(hmiddle, vmiddle);

    BulkPixelPoint p1[3] = {
      { -100, vmiddle },
      { (width * 2) / 3, -100 },
      { hmiddle, height * 2 },
    };

    BulkPixelPoint p2[3] = {
      { -2000, vmiddle },
      { width * 10, -3000 },
      { width * 5, 3000 },
    };

    const TCHAR *label;
    switch (page) {
    case 0:
      canvas.DrawSegment(center,
                         std::min(width, height) / 3,
                     Angle::Zero(), Angle::Degrees(90),
                     false);
      label = _T("segment 0-90 horizon=false");
      break;

    case 1:
      canvas.DrawSegment(center,
                         std::min(width, height) / 3,
                     Angle::Degrees(45), Angle::Degrees(180),
                     true);
      label = _T("segment 45-180 horizon=true");
      break;

    case 2:
      canvas.DrawCircle(hmiddle, vmiddle,
                        std::min(width, height) / 3);
      label = _T("circle");
      break;

    case 3:
    case 4: {
      PixelRect rc;
      rc.left = hmiddle - 50;
      rc.top = vmiddle - 20;
      rc.right = hmiddle + 50;
      rc.bottom = vmiddle + 20;
      button_renderer.DrawButton(canvas, rc, page == 4, page == 4);
      label = page == 4
        ? _T("button down=true") : _T("button down=false");
    }
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
    canvas.SetBackgroundTransparent();
    canvas.Select(normal_font);
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
  bool OnMouseDown(PixelPoint p) override {
    if (SingleWindow::OnMouseDown(p))
      return true;

    page = (page + 1) % 7;
    update();
    return true;
  }

  virtual void OnPaint(Canvas &canvas) override {
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

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override {
    switch (id) {
    case CLOSE:
      Close();
      break;

#ifndef ENABLE_OPENGL
    case BUFFER:
      buffered = !buffered;
      if (buffered) {
        WindowCanvas canvas(*this);
        buffer.Create(canvas, canvas.GetSize());
      } else
        buffer.Destroy();
      update();
      break;
#endif
    }
  }
};

static void
Main()
{
  TestWindow window;
  window.Create({250, 250});
  window.Show();

  window.RunEventLoop();
}
