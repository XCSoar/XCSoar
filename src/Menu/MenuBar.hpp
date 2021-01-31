/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Form/Button.hpp"

#include <tchar.h>

/*
    Menubar button height as a fraction of the screen height
*/
static constexpr unsigned menubar_height_scale_factor = 6;

class ContainerWindow;

/**
 * A container for menu buttons.
 */
class MenuBar {
public:
  static constexpr unsigned MAX_BUTTONS = 32;

protected:
  class Button : public ::Button {
    unsigned event;

  public:
    void SetEvent(unsigned _event) {
      event = _event;
    }

    virtual bool OnClicked();
  };

  Button buttons[MAX_BUTTONS];

public:
  MenuBar(ContainerWindow &parent, const ButtonLook &look);

public:
  void ShowButton(unsigned i, bool enabled, const TCHAR *text,
                  unsigned event);
  void HideButton(unsigned i);

  bool IsButtonEnabled(unsigned i) const {
    return buttons[i].IsEnabled();
  }

  /**
   * To be called when the parent's size changes.  Moves all buttons
   * to a new position.
   */
  void OnResize(const PixelRect &rc);
};

#endif
