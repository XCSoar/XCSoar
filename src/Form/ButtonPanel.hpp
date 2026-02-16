// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticArray.hxx"
#include "Form/Button.hpp"

#include <cassert>

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
  ButtonPanel(ContainerWindow &parent, const ButtonLook &look) noexcept;
  ~ButtonPanel() noexcept;

  /**
   * On devices without a touch screen, enable button selection with
   * KEY_LEFT / KEY_RIGHT.  That allows navigating a ListControl while
   * allowing the user to select an action on a list item.
   */
  void EnableCursorSelection(unsigned _index=0) noexcept {
    assert(selected_index < 0);
    assert(_index < buttons.size());

    selected_index = _index;
    buttons[selected_index]->SetSelected(true);
  }

  const ButtonLook &GetLook() const noexcept {
    return look;
  }

  void SetDefaultHidden() noexcept {
    style.Hide();
  }

  Button *Add(std::unique_ptr<ButtonRenderer> &&renderer,
              Button::Callback callback) noexcept;

  Button *Add(const char *caption, Button::Callback callback) noexcept;

  /**
   * Add a symbol button.  The caption is one of the "special"
   * #WndSymbolButton strings.
   */
  Button *AddSymbol(const char *caption, Button::Callback callback) noexcept;

  /**
   * Assign a hot key to the most recently added button.
   */
  void AddKey(unsigned key_code) noexcept;

  /**
   * Call this after all buttons have been added or after the parent
   * window has been resized.
   *
   * @return the remaining rectangle
   */
  PixelRect UpdateLayout(PixelRect rc) noexcept;
  PixelRect UpdateLayout() noexcept;

  /**
   * Move buttons in columns on the left.
   */
  PixelRect LeftLayout(PixelRect rc) noexcept;
  PixelRect LeftLayout() noexcept;

  /**
   * Move buttons to rows on the bottom.
   */
  PixelRect BottomLayout(PixelRect rc) noexcept;
  PixelRect BottomLayout() noexcept;

  void ShowAll() noexcept;
  void HideAll() noexcept;

  [[gnu::pure]]
  bool HasFocus() const noexcept;

  /**
   * Handle a hot key.
   *
   * @return true if the event has been handled
   */
  bool KeyPress(unsigned key_code) noexcept;

private:
  [[gnu::pure]]
  unsigned Width(unsigned i) const noexcept;

  /**
   * Check how many buttons fit into a row, starting at the given
   * offset.
   *
   * @param start the first button index in this row
   * @param total_width the total width of the panel in pixels
   * @return the first button index not in this row
   */
  [[gnu::pure]]
  unsigned FitButtonRow(unsigned start, unsigned total_width) const noexcept;

  [[gnu::pure]]
  unsigned RangeMaxWidth(unsigned start, unsigned end) const noexcept;

  PixelRect VerticalRange(PixelRect rc, unsigned start, unsigned end) noexcept;

  PixelRect HorizontalRange(PixelRect rc,
                            unsigned start, unsigned end) noexcept;

  void SetSelectedIndex(unsigned _index) noexcept;
  bool SelectPrevious() noexcept;
  bool SelectNext() noexcept;
};
