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

#ifndef XCSOAR_SCREEN_TEXT_WINDOW_HXX
#define XCSOAR_SCREEN_TEXT_WINDOW_HXX

#include "Screen/Window.hpp"

#ifdef ENABLE_SDL
#include <tstring.hpp>
#include <algorithm>
#endif

class TextWindowStyle : public WindowStyle {
public:
  TextWindowStyle() {}
  TextWindowStyle(const WindowStyle other):WindowStyle(other) {}

  void left() {
#ifndef ENABLE_SDL
    style |= SS_LEFT;
#endif
  }

  void right() {
#ifndef ENABLE_SDL
    style |= SS_RIGHT;
#endif
  }

  void center() {
#ifndef ENABLE_SDL
    style |= SS_CENTER;
#endif
  }

  void notify() {
#ifndef ENABLE_SDL
    style |= SS_NOTIFY;
#endif
  }
};

/**
 * A window which renders static text.
 */
class TextWindow : public Window {
#ifdef ENABLE_SDL
  tstring text;
#endif

public:
  void set(ContainerWindow &parent, const TCHAR *text,
           int left, int top, unsigned width, unsigned height,
           const TextWindowStyle style=TextWindowStyle());

  void set_text(const TCHAR *_text) {
    assert_none_locked();
    assert_thread();

#ifdef ENABLE_SDL
    if (_text != NULL)
      text = _text;
    else
      text.clear();
    invalidate();
#else /* !ENABLE_SDL */
    ::SetWindowText(hWnd, _text);

#ifdef _WIN32_WCE
    ::UpdateWindow(hWnd);
#endif
#endif /* !ENABLE_SDL */
  }

#ifdef ENABLE_SDL
protected:
  virtual void on_paint(Canvas &canvas);
#endif /* ENABLE_SDL */
};

#endif
