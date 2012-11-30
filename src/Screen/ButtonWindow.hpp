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

#ifndef XCSOAR_SCREEN_BUTTON_WINDOW_HXX
#define XCSOAR_SCREEN_BUTTON_WINDOW_HXX

#include <Screen/Window.hpp>
#include "Util/tstring.hpp"

#include <winuser.h>

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

  void EnableCustomPainting() {
    WindowStyle::EnableCustomPainting();
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
  void Create(ContainerWindow &parent, const TCHAR *text, unsigned id,
              const PixelRect &rc,
              const ButtonWindowStyle style=ButtonWindowStyle());

  void Create(ContainerWindow &parent, const TCHAR *text,
              const PixelRect &rc,
              const ButtonWindowStyle style=ButtonWindowStyle()) {
    Create(parent, text, 0, rc, style);
  }

  unsigned GetID() const {
    return id;
  }

  void SetText(const TCHAR *_text) {
    AssertNoneLocked();
    AssertThread();

    text = _text;
    Invalidate();
  }

  const tstring &GetText() const {
    return text;
  }

protected:
  void SetDown(bool _down);

public:
  bool IsDown() const {
    return down;
  }

  /**
   * The button was clicked, and its action shall be triggered.
   */
  virtual bool OnClicked();

protected:
  bool OnKeyCheck(unsigned key_code) const;
  virtual bool OnKeyDown(unsigned key_code);
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys);
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y);
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y);
  virtual void OnSetFocus();
  virtual void OnKillFocus();
  virtual void OnCancelMode() gcc_override;
  virtual void OnPaint(Canvas &canvas);
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
     * ButtonWindow::OnClicked().
     */
    COMMAND_BOUNCE_ID = 0xbeef,
  };

public:
  void Create(ContainerWindow &parent, const TCHAR *text, unsigned id,
              const PixelRect &rc,
              const WindowStyle style);

  void Create(ContainerWindow &parent, const TCHAR *text,
              const PixelRect &rc,
              const WindowStyle style) {
    Create(parent, text, COMMAND_BOUNCE_ID, rc, style);
  }

  gcc_pure
  unsigned GetID() const {
    return ::GetWindowLong(hWnd, GWL_ID);
  }

  /**
   * The button was clicked, and its action shall be triggered.
   */
  virtual bool OnClicked();

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
  void Create(ContainerWindow &parent, const TCHAR *text, unsigned id,
              const PixelRect &rc,
              const ButtonWindowStyle style=ButtonWindowStyle()) {
    BaseButtonWindow::Create(parent, text, id, rc, style);
  }

  void Create(ContainerWindow &parent, const TCHAR *text,
              const PixelRect &rc,
              const ButtonWindowStyle style=ButtonWindowStyle()) {
    BaseButtonWindow::Create(parent, text, rc, style);
  }

  bool IsDown() const {
    AssertNoneLocked();
    AssertThread();

    return (Button_GetState(hWnd) & BST_PUSHED) != 0;
  }

  void SetText(const TCHAR *text);

  const tstring GetText() const;

protected:
  virtual bool OnKeyCheck(unsigned key_code) const;
  virtual bool OnKeyDown(unsigned key_code);
};

#endif /* USE_GDI */

#endif
