/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "Screen/TopWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Blank.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Interface.hpp"
#include "InfoBoxLayout.h"

#ifdef HAVE_BLANK
int DisplayTimeOut;
#endif

int InfoBoxLayout::scale = 1;
double InfoBoxLayout::dscale = 1.0;
bool InfoBoxLayout::IntScaleFlag;

HINSTANCE CommonInterface::hInst;
void XCSoarInterface::InterfaceTimeoutReset(void) {}

class TestWindow : public TopWindow {
  ButtonWindow buffer_button, close_button;
  unsigned page;
  bool buffered;
  BufferCanvas buffer;

  enum {
    ID_START = 100,
    ID_BUFFER,
    ID_CLOSE
  };

public:
  TestWindow():page(0), buffered(false) {}

  static bool register_class(HINSTANCE hInstance) {
#ifdef ENABLE_SDL
    return true;
#else /* !ENABLE_SDL */
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Window::WndProc;
    wc.cbClsExtra = 0;
#ifdef WINDOWSPC
    wc.cbWndExtra = 0;
#else
    WNDCLASS dc;
    GetClassInfo(hInstance, TEXT("DIALOG"), &dc);
    wc.cbWndExtra = dc.cbWndExtra ;
#endif
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
    TopWindow::set(_T("RunCanvas"), _T("RunCanvas"),
                left, top, width, height);

    RECT rc = get_client_rect();

    buffer_button.set(*this, _T("Buffer"), ID_BUFFER, 5, rc.bottom - 30, 65, 25);
    buffer_button.install_wndproc();

    close_button.set(*this, _T("Close"), ID_CLOSE, rc.right - 70, rc.bottom - 30, 65, 25);
    close_button.install_wndproc();
  }

private:
  void paint(Canvas &canvas) {
    canvas.hollow_brush();
    canvas.black_pen();

    Brush red_brush(Color(255, 0, 0));

    POINT p1[3] = {
      { -100, get_vmiddle() },
      { (get_width() * 2) / 3, -100 },
      { get_hmiddle(), get_height() * 2 },
    };

    POINT p2[3] = {
      { -2000, get_vmiddle() },
      { get_width() * 10, -3000 },
      { get_width() * 5, 3000 },
    };

    const TCHAR *label;
    switch (page) {
    case 0:
      canvas.segment(get_hmiddle(), get_vmiddle(),
                     min(get_width(), get_height()) / 3,
                     get_client_rect(),
                     0, 90,
                     false);
      label = _T("segment 0-90 horizon=false");
      break;

    case 1:
      canvas.segment(get_hmiddle(), get_vmiddle(),
                     min(get_width(), get_height()) / 3,
                     get_client_rect(),
                     45, 180,
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
      RECT rc;
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
    canvas.text(5, 25,
                buffered ? _T("buffered") : _T("not buffered"));
  }

  void update() {
    RECT rc = get_client_rect();

    if (buffered) {
      buffer.white_brush();
      buffer.white_pen();
      buffer.rectangle(rc.left, rc.top, rc.right, rc.bottom);

      paint(buffer);
    }

    PaintWindow::update(rc);
  }

protected:
  virtual bool on_destroy(void) {
    TopWindow::on_destroy();
    ::PostQuitMessage(0);
    return true;
  }

  virtual bool on_mouse_down(int x, int y) {
    page = (page + 1) % 7;
    update();
    return true;
  }

  virtual bool on_command(unsigned id, unsigned code) {
    switch (id) {
    case ID_CLOSE:
      close();
      return true;

    case ID_BUFFER:
      buffered = !buffered;
      if (buffered)
        buffer.set(get_canvas(), get_width(), get_height());
      else
        buffer.reset();
      update();
      return true;
    }

    return TopWindow::on_command(id, code);
  }

  /*
  virtual bool on_erase(Canvas &canvas) {
    canvas.white_brush();
    canvas.white_pen();

    RECT rc = get_client_rect();
    canvas.rectangle(rc.left, rc.top, rc.right, rc.bottom);
    return true;
  }
  */

  virtual void on_paint(Canvas &canvas) {
    if (!buffered) {
      // due to a limitation of our PaintCanvas class, we cannot rely on
      // background erasing with on_erase()
      canvas.white_brush();
      canvas.white_pen();

      RECT rc = get_client_rect();
      canvas.rectangle(rc.left, rc.top, rc.right, rc.bottom);

      paint(canvas);
    } else
      canvas.copy(buffer);
  }
};

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPTSTR lpCmdLine, int nCmdShow)
#endif
{
  InitSineTable();

#ifdef WIN32
  CommonInterface::hInst = hInstance;

  TestWindow::register_class(hInstance);
#endif

  TestWindow window;
  window.set(0, 0, 250, 250);
  window.show();

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}
