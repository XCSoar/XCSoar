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

#ifndef XCSOAR_FORM_BUTTON_PANEL_HPP
#define XCSOAR_FORM_BUTTON_PANEL_HPP

#include "Util/StaticArray.hpp"
#include "Form/Button.hpp"

class ButtonPanel {
  ContainerWindow &parent;
  const ButtonLook &look;
  WindowStyle style;

  StaticArray<Button *, 8u> buttons;

  /**
   * Map key codes to the button that "owns" it.  Used by KeyPress().
   */
  unsigned keys[8u];

public:
  ButtonPanel(ContainerWindow &parent, const ButtonLook &look);
  ~ButtonPanel();

  const ButtonLook &GetLook() const {
    return look;
  }

  void SetDefaultHidden() {
    style.Hide();
  }

  Button *Add(ButtonRenderer *renderer,
              ActionListener &listener, int id);

  Button *Add(const TCHAR *caption,
              ActionListener &listener, int id);

  /**
   * Add a symbol button.  The caption is one of the "special"
   * #WndSymbolButton strings.
   */
  Button *AddSymbol(const TCHAR *caption,
                    ActionListener &listener, int id);

  /**
   * Assign a hot key to the most recently added button.
   */
  void AddKey(unsigned key_code);

  /**
   * Wrapper for AddKey() which is Altair specific; it a no-op on all
   * other platforms.
   */
  void AddAltairKey(gcc_unused unsigned key_code) {
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

private:
  gcc_pure
  unsigned Width(unsigned i) const;

  /**
   * Check how many buttons fit into a row, starting at the given
   * offset.
   *
   * @param start the first button index in this row
   * @param total_width the total width of the panel in pixels
   * @return the first button index not in this row
   */
  gcc_pure
  unsigned FitButtonRow(unsigned start, unsigned total_width) const;

  gcc_pure
  unsigned RangeMaxWidth(unsigned start, unsigned end) const;

  PixelRect VerticalRange(PixelRect rc, unsigned start, unsigned end);

  PixelRect HorizontalRange(PixelRect rc, unsigned start, unsigned end);
};

#endif
