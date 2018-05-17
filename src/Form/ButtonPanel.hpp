/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Util/StaticArray.hxx"
#include "Form/Button.hpp"

#include <assert.h>

class ButtonPanel {
  ContainerWindow &parent;
  const ButtonLook &look;
  WindowStyle style;

  StaticArray<Button *, 8u> buttons;

  /**
   * Map key codes to the button that "owns" it.  Used by KeyPress().
   */
  unsigned keys[8u];

  /**
   * The button currently selected with KEY_LEFT / KEY_RIGHT on
   * devices without a touch screen.  If negative, then cursor
   * selection is disabled in this ButtonPanel.  Can be enabled with
   * EnableCursorSelection().
   */
  int selected_index;

public:
  ButtonPanel(ContainerWindow &parent, const ButtonLook &look);
  ~ButtonPanel();

  /**
   * On devices without a touch screen, enable button selection with
   * KEY_LEFT / KEY_RIGHT.  That allows navigating a ListControl while
   * allowing the user to select an action on a list item.
   */
  void EnableCursorSelection(unsigned _index=0) {
    assert(selected_index < 0);
    assert(_index < buttons.size());

    selected_index = _index;
    buttons[selected_index]->SetSelected(true);
  }

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

  void SetSelectedIndex(unsigned _index);
  bool SelectPrevious();
  bool SelectNext();
};

#endif
