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

#ifndef XCSOAR_FORM_BUTTON_PANEL_HPP
#define XCSOAR_FORM_BUTTON_PANEL_HPP

#include "Util/StaticArray.hpp"
#include "Form/Button.hpp"

class ButtonPanel {
  ContainerWindow &parent;
  const DialogLook &look;
  ButtonWindowStyle style;

  StaticArray<WndButton *, 8u> buttons;

  PixelRect rc, remaining;

  bool vertical;

public:
  ButtonPanel(ContainerWindow &parent, const DialogLook &look);
  ~ButtonPanel();

  void Resized(const PixelRect &area);

  const PixelRect &GetRemainingRect() const {
    return remaining;
  }

  WndButton *Add(const TCHAR *caption,
                 WndButton::ClickNotifyCallback_t callback);

protected:
  PixelRect GetButtonRect(unsigned i) const;
  void MoveButtons();
};

#endif
