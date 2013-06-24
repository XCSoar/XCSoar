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

#ifndef XCSOAR_FORM_BUTTON_PANEL_HPP
#define XCSOAR_FORM_BUTTON_PANEL_HPP

#include "Util/StaticArray.hpp"
#include "Form/Button.hpp"

class ButtonPanel {
  ContainerWindow &parent;
  const DialogLook &look;
  ButtonWindowStyle style;

  StaticArray<WndButton *, 8u> buttons;

  /**
   * Map key codes to the button that "owns" it.  Used by KeyPress().
   */
  unsigned keys[8u];

public:
  ButtonPanel(ContainerWindow &parent, const DialogLook &look);
  ~ButtonPanel();

  void SetDefaultHidden() {
    style.Hide();
  }

  WndButton *Add(tstring::const_pointer caption,
                 ActionListener &listener, int id);

  /**
   * Add a symbol button.  The caption is one of the "special"
   * #WndSymbolButton strings.
   */
  WndButton *AddSymbol(tstring::const_pointer caption,
                       ActionListener &listener, int id);

  /**
   * Assign a hot key to the most recently added button.
   */
  void AddKey(unsigned key_code);

  /**
   * Wrapper for AddKey() which is Altair specific; it a no-op on all
   * other platforms.
   */
  void AddAltairKey(unsigned key_code) {
#ifdef GNAV
    AddKey(key_code);
#endif
  }

  /**
   * Call this after all buttons have been added or after the parent
   * window has been resized.
   *
   * @return the remaining rectangle
   */
  PixelRect UpdateLayout(PixelRect rc);
  PixelRect UpdateLayout();

  /**
   * Move buttons in columns on the left.
   */
  PixelRect LeftLayout(PixelRect rc);
  PixelRect LeftLayout();

  /**
   * Move buttons to rows on the bottom.
   */
  PixelRect BottomLayout(PixelRect rc);
  PixelRect BottomLayout();

  void ShowAll();
  void HideAll();

  /**
   * Handle a hot key.
   *
   * @return true if the event has been handled
   */
  bool KeyPress(unsigned key_code);

protected:
  gcc_pure
  UPixelScalar Width(unsigned i) const;

  gcc_pure
  UPixelScalar RangeMaxWidth(unsigned start, unsigned end) const;

  PixelRect VerticalRange(PixelRect rc, unsigned start, unsigned end);

  PixelRect HorizontalRange(PixelRect rc, unsigned start, unsigned end);
};

#endif
