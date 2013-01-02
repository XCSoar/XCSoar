/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

  void EnableCustomPainting() {
    WindowStyle::EnableCustomPainting();
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

  void Create(ContainerWindow &parent, tstring::const_pointer text,
              unsigned id,
              const PixelRect &rc,
              const CheckBoxStyle style=CheckBoxStyle());

  void Create(ContainerWindow &parent, tstring::const_pointer text,
              const PixelRect &rc,
              const CheckBoxStyle style=CheckBoxStyle()) {
    Create(parent, text, 0, rc, style);
  }

  bool GetState() const {
    return checked;
  }

  unsigned GetID() const {
    return id;
  }

  void SetState(bool value);

protected:
  void SetPressed(bool value);

  virtual bool OnKeyDown(unsigned key_code);
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys);
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y);
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y);
  virtual void OnSetFocus();
  virtual void OnKillFocus();
  virtual void OnCancelMode() gcc_override;
  virtual void OnPaint(Canvas &canvas);

  virtual bool OnClicked();
};

#else /* USE_GDI */

#include "Screen/ButtonWindow.hpp"

/**
 * A check box.
 */
class CheckBox : public BaseButtonWindow {
public:
  void Create(ContainerWindow &parent, tstring::const_pointer text,
              unsigned id,
              const PixelRect &rc,
              const CheckBoxStyle style=CheckBoxStyle()) {
    BaseButtonWindow::Create(parent, text, id, rc, style);
  }

  void Create(ContainerWindow &parent, tstring::const_pointer text,
              const PixelRect &rc,
              const CheckBoxStyle style=CheckBoxStyle()) {
    BaseButtonWindow::Create(parent, text, rc, style);
  }

  bool GetState() const {
    return SendMessage(hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
  }

  void SetState(bool value) {
    SendMessage(hWnd, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
  }
};

#endif /* USE_GDI */

#endif
