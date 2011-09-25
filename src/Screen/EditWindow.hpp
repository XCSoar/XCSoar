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

#ifndef XCSOAR_SCREEN_EDIT_WINDOW_HXX
#define XCSOAR_SCREEN_EDIT_WINDOW_HXX

#include "Screen/Window.hpp"

#ifndef USE_GDI
#include <tstring.hpp>
#include <algorithm>
#endif

class EditWindowStyle : public WindowStyle {
public:
  bool is_read_only;

public:
#ifndef USE_GDI
  EditWindowStyle():is_read_only(false) {
    text_style |= DT_LEFT | DT_VCENTER;
  }

  EditWindowStyle(const WindowStyle other)
    :WindowStyle(other), is_read_only(false) {
    text_style |= DT_LEFT | DT_VCENTER;
  }
#else
  EditWindowStyle() {
    style |= ES_LEFT | ES_AUTOHSCROLL;
  }

  EditWindowStyle(const WindowStyle other):WindowStyle(other) {
    style |= ES_LEFT | ES_AUTOHSCROLL;
  }
#endif

  void read_only() {
#ifndef USE_GDI
    is_read_only = true;
#else
    style |= ES_READONLY;
#endif
  }

  void multiline() {
#ifndef USE_GDI
    text_style &= ~DT_VCENTER;
    text_style |= DT_WORDBREAK;
#else
    style &= ~ES_AUTOHSCROLL;
    style |= ES_MULTILINE;
#endif
  }

  void center() {
#ifndef USE_GDI
    text_style &= ~DT_LEFT;
    text_style |= DT_CENTER;
#else
    style &= ~ES_LEFT;
    style |= ES_CENTER;
#endif
  }
};

/**
 * A simple text editor widget.
 */
class EditWindow : public Window {
#ifndef USE_GDI
  bool read_only;

  tstring value;
#endif

public:
  void set(ContainerWindow &parent, PixelScalar left, PixelScalar top,
           UPixelScalar width, UPixelScalar height,
           const EditWindowStyle style);

  unsigned get_row_count() const {
    assert_none_locked();

#ifndef USE_GDI
    const TCHAR *str = value.c_str();
    int row_count = 1;

    while ((str = strchr(str, _T('\n'))) != NULL) {
      str++;
      row_count++;
    }
    return row_count;
#else /* USE_GDI */
    return ::SendMessage(hWnd, EM_GETLINECOUNT, 0, 0);
#endif /* USE_GDI */
  }

  void set_text(const TCHAR *text) {
    assert_none_locked();

#ifndef USE_GDI
    if (text != NULL)
      value = text;
    else
      value.clear();
    invalidate();
#else
    ::SetWindowText(hWnd, text);
#endif
  }

  void get_text(TCHAR *text, size_t max_length) {
#ifndef USE_GDI
    value.copy(text, std::min(max_length - 1, value.length()));
#else
    ::GetWindowText(hWnd, text, max_length);
#endif
  }

  void set_read_only(bool value) {
    assert_none_locked();

#ifndef USE_GDI
    // XXX
#else
    ::SendMessage(hWnd, EM_SETREADONLY, (WPARAM)(BOOL)value, 0L);
#endif
  }

  bool is_read_only() const {
#ifndef USE_GDI
    return read_only;
#else
    return (get_window_style() & ES_READONLY) != 0;
#endif
  }

  void set_selection(int start, int end) {
    assert_none_locked();

#ifndef USE_GDI
    // XXX
#else
    ::SendMessage(hWnd, EM_SETSEL, (WPARAM)start, (LPARAM)end);
#endif
  }

  void set_selection() {
#ifndef USE_GDI
    // XXX
#else
    set_selection(0, -1);
#endif
  }

#ifndef USE_GDI
protected:
  virtual void on_paint(Canvas &canvas);
#endif /* !USE_GDI */
};

#endif
