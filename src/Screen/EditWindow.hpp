/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Util/tstring.hpp"
#include <algorithm>
#endif

#include <winuser.h>

class EditWindowStyle : public WindowStyle {
#ifndef USE_GDI
public:
  bool is_read_only;
#endif

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

  void SetReadOnly() {
#ifndef USE_GDI
    is_read_only = true;
#else
    style |= ES_READONLY;
#endif
  }

  void SetMultiLine() {
#ifndef USE_GDI
    text_style &= ~DT_VCENTER;
    text_style |= DT_WORDBREAK;
#else
    style &= ~ES_AUTOHSCROLL;
    style |= ES_MULTILINE;
#endif
  }

  void SetCenter() {
#ifndef USE_GDI
    text_style &= ~DT_LEFT;
    text_style |= DT_CENTER;
#else
    style &= ~ES_LEFT;
    style |= ES_CENTER;
#endif
  }

  void SetVerticalCenter() {
#ifndef USE_GDI
    text_style |= DT_VCENTER;
#else
    // TODO
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

  /**
   * The first visible line.
   */
  unsigned origin;
#endif

public:
  void Create(ContainerWindow &parent, PixelRect rc,
              const EditWindowStyle style);

#ifndef USE_GDI
  gcc_pure
  unsigned GetVisibleRows() const {
    assert(IsMultiLine());

    return GetHeight() / GetFont().GetHeight();
  }
#endif

  gcc_pure
  unsigned GetRowCount() const {
    AssertNoneLocked();

#ifndef USE_GDI
    const TCHAR *str = value.c_str();
    int row_count = 1;

    while ((str = _tcschr(str, _T('\n'))) != NULL) {
      str++;
      row_count++;
    }
    return row_count;
#else /* USE_GDI */
    return ::SendMessage(hWnd, EM_GETLINECOUNT, 0, 0);
#endif /* USE_GDI */
  }

  void SetText(const TCHAR *text);

  void GetText(TCHAR *text, size_t max_length) {
#ifndef USE_GDI
    value.copy(text, std::min(max_length - 1, value.length()));
#else
    ::GetWindowText(hWnd, text, max_length);
#endif
  }

  void SetReadOnly(bool value) {
    AssertNoneLocked();

#ifndef USE_GDI
    read_only = value;
    Invalidate();
#else
    ::SendMessage(hWnd, EM_SETREADONLY, (WPARAM)(BOOL)value, 0L);
#endif
  }

  bool IsReadOnly() const {
#ifndef USE_GDI
    return read_only;
#else
    return (GetStyle() & ES_READONLY) != 0;
#endif
  }

  bool IsMultiLine() const {
#ifndef USE_GDI
    return (GetTextStyle() & DT_WORDBREAK) != 0;
#else
    return (GetStyle() & ES_MULTILINE) != 0;
#endif
  }

  void Select(int start, int end) {
    AssertNoneLocked();

#ifndef USE_GDI
    // XXX
#else
    ::SendMessage(hWnd, EM_SETSEL, (WPARAM)start, (LPARAM)end);
#endif
  }

  void SelectAll() {
#ifndef USE_GDI
    // XXX
#else
    Select(0, -1);
#endif
  }

  /**
   * Scroll the contents of a multi-line control by the specified
   * number of lines.
   */
  void ScrollVertically(int delta_lines);

#ifndef USE_GDI
protected:
  virtual void OnResize(UPixelScalar width, UPixelScalar height);
  virtual void OnPaint(Canvas &canvas);
  virtual bool OnKeyCheck(unsigned key_code) const;
  virtual bool OnKeyDown(unsigned key_code);
#endif /* !USE_GDI */
};

#endif
