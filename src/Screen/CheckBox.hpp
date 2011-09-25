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

#ifndef XCSOAR_SCREEN_CHECK_BOX_WINDOW_HXX
#define XCSOAR_SCREEN_CHECK_BOX_WINDOW_HXX

#include "Screen/Window.hpp"

class CheckBoxStyle : public WindowStyle {
public:
  CheckBoxStyle() {
#ifdef USE_GDI
    style |= BS_CHECKBOX | BS_AUTOCHECKBOX;
#endif
  }

  CheckBoxStyle(const WindowStyle _style):WindowStyle(_style) {
#ifdef USE_GDI
    style |= BS_CHECKBOX | BS_AUTOCHECKBOX;
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
#include "Util/tstring.hpp"

class CheckBox : public PaintWindow {
  tstring text;
  unsigned id;
  bool checked, dragging, pressed;

public:
  CheckBox():checked(false), dragging(false), pressed(false) {}

  void set(ContainerWindow &parent, const TCHAR *text, unsigned id,
           PixelScalar left, PixelScalar top,
           UPixelScalar width, UPixelScalar height,
           const CheckBoxStyle style=CheckBoxStyle());

  void set(ContainerWindow &parent, const TCHAR *text,
           PixelScalar left, PixelScalar top,
           UPixelScalar width, UPixelScalar height,
           const CheckBoxStyle style=CheckBoxStyle()) {
    set(parent, text, 0, left, top, width, height, style);
  }

  bool get_checked() const {
    return checked;
  }

  void set_checked(bool value);

protected:
  void set_pressed(bool value);

  virtual bool on_key_down(unsigned key_code);
  virtual bool on_mouse_move(PixelScalar x, PixelScalar y, unsigned keys);
  virtual bool on_mouse_down(PixelScalar x, PixelScalar y);
  virtual bool on_mouse_up(PixelScalar x, PixelScalar y);
  virtual bool on_setfocus();
  virtual bool on_killfocus();
  virtual bool on_cancel_mode();
  virtual void on_paint(Canvas &canvas);

  virtual bool on_clicked();
};

#else /* USE_GDI */

#include "Screen/ButtonWindow.hpp"

/**
 * A check box.
 */
class CheckBox : public BaseButtonWindow {
public:
  void set(ContainerWindow &parent, const TCHAR *text, unsigned id,
           PixelScalar left, PixelScalar top,
           UPixelScalar width, UPixelScalar height,
           const CheckBoxStyle style=CheckBoxStyle()) {
    BaseButtonWindow::set(parent, text, id, left, top, width, height, style);
  }

  void set(ContainerWindow &parent, const TCHAR *text,
           PixelScalar left, PixelScalar top,
           UPixelScalar width, UPixelScalar height,
           const CheckBoxStyle style=CheckBoxStyle()) {
    BaseButtonWindow::set(parent, text, left, top, width, height, style);
  }

  bool get_checked() const {
    return SendMessage(hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
  }

  void set_checked(bool value) {
    SendMessage(hWnd, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
  }
};

#endif /* USE_GDI */

#endif
