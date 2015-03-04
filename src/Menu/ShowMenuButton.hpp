/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_SHOW_MENU_BUTTON_HPP
#define XCSOAR_SHOW_MENU_BUTTON_HPP

#include "Screen/ButtonWindow.hpp"

#include <tchar.h>

class ShowMenuButton : public ButtonWindow {
public:
  void Create(ContainerWindow &parent, const PixelRect &rc,
              ButtonWindowStyle style=ButtonWindowStyle()) {
    style.EnableCustomPainting();
    ButtonWindow::Create(parent, _T(""), rc, style);
  }

protected:
  /* virtual methods from class ButtonWindow */
  bool OnClicked() override;

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) override;
};

#endif
