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

#ifndef XCSOAR_SCREEN_BUTTON_WINDOW_HXX
#define XCSOAR_SCREEN_BUTTON_WINDOW_HXX

#include <Screen/Window.hpp>
#include "Util/tstring.hpp"

class ButtonWindowStyle : public WindowStyle {
public:
  ButtonWindowStyle() {
#ifndef USE_GDI
    text_style |= DT_CENTER | DT_VCENTER | DT_WORDBREAK;
#else
    style |= BS_PUSHBUTTON | BS_CENTER | BS_VCENTER;
#endif
  }

  ButtonWindowStyle(const WindowStyle other):WindowStyle(other) {
#ifndef USE_GDI
    text_style |= DT_CENTER | DT_VCENTER | DT_WORDBREAK;
#else
    style |= BS_PUSHBUTTON | BS_CENTER | BS_VCENTER;
#endif
  }

  void multiline() {
#ifdef USE_GDI
    style |= BS_MULTILINE;
#endif
  }

  void enable_custom_painting() {
    WindowStyle::enable_custom_painting();
#ifdef USE_GDI
    style |= BS_OWNERDRAW;
#endif
  }
};

#ifndef USE_GDI

#include "Screen/PaintWindow.hpp"

/**
 * A clickable button.
 */
class ButtonWindow : public PaintWindow
{
  tstring text;
  unsigned id;
  bool dragging, down;

public:
  ButtonWindow():dragging(false), down(false) {}

public:
  void set(ContainerWindow &parent, const TCHAR *text, unsigned id,
           int left, int top, unsigned width, unsigned height,
           const ButtonWindowStyle style=ButtonWindowStyle());

  void set(ContainerWindow &parent, const TCHAR *text,
           int left, int top, unsigned width, unsigned height,
           const ButtonWindowStyle style=ButtonWindowStyle()) {
    set(parent, text, 0, left, top, width, height, style);
  }

  void set_text(const TCHAR *_text) {
    assert_none_locked();
    assert_thread();

    text = _text;
    invalidate();
  }

  const tstring &get_text() const {
    return text;
  }

protected:
  void set_down(bool _down);

public:
  bool is_down() const {
    return down;
  }

  /**
   * The button was clicked, and its action shall be triggered.
   */
  virtual bool on_clicked();

protected:
  bool on_key_check(unsigned key_code) const;
  virtual bool on_key_down(unsigned key_code);
  virtual bool on_mouse_move(int x, int y, unsigned keys);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
  virtual bool on_setfocus();
  virtual bool on_killfocus();
  virtual bool on_cancel_mode();
  virtual void on_paint(Canvas &canvas);
};

#else /* USE_GDI */

#include "Screen/Window.hpp"

#include <windowsx.h>
#include <tchar.h>

/**
 * A base class for WC_BUTTON windows.
 */
class BaseButtonWindow : public Window {
public:
  enum {
    /**
     * On WIN32, a WM_COMMAND/BN_CLICKED message with this id will be
     * bounced back to the originating child
     * ButtonWindow::on_clicked().
     */
    COMMAND_BOUNCE_ID = 0xbeef,
  };

public:
  void set(ContainerWindow &parent, const TCHAR *text, unsigned id,
           int left, int top, unsigned width, unsigned height,
           const WindowStyle style);

  void set(ContainerWindow &parent, const TCHAR *text,
           int left, int top, unsigned width, unsigned height,
           const WindowStyle style) {
    set(parent, text, COMMAND_BOUNCE_ID,
        left, top, width, height, style);
  }

  gcc_pure
  unsigned GetID() const {
    return ::GetWindowLong(hWnd, GWL_ID);
  }

  /**
   * The button was clicked, and its action shall be triggered.
   */
  virtual bool on_clicked();

protected:
  /**
   * Synthesise a click.
   */
  void Click();
};

/**
 * A clickable button.
 */
class ButtonWindow : public BaseButtonWindow {
public:
  void set(ContainerWindow &parent, const TCHAR *text, unsigned id,
           int left, int top, unsigned width, unsigned height,
           const ButtonWindowStyle style=ButtonWindowStyle()) {
    BaseButtonWindow::set(parent, text, id, left, top, width, height, style);
  }

  void set(ContainerWindow &parent, const TCHAR *text,
           int left, int top, unsigned width, unsigned height,
           const ButtonWindowStyle style=ButtonWindowStyle()) {
    BaseButtonWindow::set(parent, text, left, top, width, height, style);
  }

  bool is_down() const {
    assert_none_locked();
    assert_thread();

    return (Button_GetState(hWnd) & BST_PUSHED) != 0;
  }

  void set_text(const TCHAR *text);

  const tstring get_text() const;

protected:
  virtual bool on_key_check(unsigned key_code) const;
  virtual bool on_key_down(unsigned key_code);
};

#endif /* USE_GDI */

#endif
