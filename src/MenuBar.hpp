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

#ifndef XCSOAR_MENU_BAR_HPP
#define XCSOAR_MENU_BAR_HPP

#include "Screen/ButtonWindow.hpp"

#include <tchar.h>

class Window;
class ContainerWindow;

/**
 * A container for menu buttons.
 */
class MenuBar {
public:
  enum {
    MAX_BUTTONS = 32,
  };

protected:
  class Button : public ButtonWindow {
    unsigned index;

  public:
    virtual bool on_clicked();

  public:
    void set(ContainerWindow &parent, const TCHAR *text, unsigned _index,
             PixelScalar left, PixelScalar top,
             UPixelScalar width, UPixelScalar height,
             ButtonWindowStyle style=ButtonWindowStyle()) {
      index = _index;

      ButtonWindow::set(parent, text, left, top, width, height, style);
    }

#ifdef USE_GDI
  protected:
    virtual LRESULT on_message(HWND hWnd, UINT message,
                               WPARAM wParam, LPARAM lParam);
#endif
  };

  Button buttons[MAX_BUTTONS];

public:
  MenuBar(ContainerWindow &parent);

public:
  void SetFont(const Font &font);
  void ShowButton(unsigned i, bool enabled, const TCHAR *text);
  void HideButton(unsigned i);

  bool IsButtonEnabled(unsigned i) const {
    return buttons[i].is_enabled();
  }
};

#endif
