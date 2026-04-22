// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Point.hpp"
#include "Widget.hpp"
#include "Form/CharacterButton.hpp"
#include "Form/Button.hpp"

struct ButtonLook;
class ContainerWindow;
class Window;

class KeyboardWidget : public NullWidget {
public:
  typedef bool (*OnCharacterCallback_t)(unsigned ch);

protected:
  static constexpr unsigned MAX_BUTTONS = 40;

  const ButtonLook &look;

  OnCharacterCallback_t on_character;

  PixelSize button_size;

  unsigned num_buttons;
  CharacterButton buttons[MAX_BUTTONS];

  Button shift_button;
  bool shift_state;

  const bool show_shift_button;

  /**
   * Client area of the text-entry dialog; used for focus lookup when
   * moving between on-screen key buttons with the hardware cursor keys.
   */
  ContainerWindow *parent_container = nullptr;

  /**
   * When @c KEY_UP moves from the number row to the on-screen backspace, we
   * remember the digit index (0..9) so @c KEY_DOWN from that button can return
   * to the same key.
   */
  int number_row_before_backspace = -1;

public:
  KeyboardWidget(const ButtonLook &_look,
                 OnCharacterCallback_t _on_character,
                 bool _show_shift_button,
                 bool _default_shift_state = true)
    :look(_look), on_character(_on_character), num_buttons(0),
     shift_state(_default_shift_state),
     show_shift_button(_show_shift_button) {}

  /**
   * Show only the buttons representing the specified character list.
   */
  void SetAllowedCharacters(const char *allowed);

  /**
   * Move focus to the on-screen @em Space key (inverse of @c KEY_DOWN
   * from that key to the action row in text entry).
   */
  bool FocusSpaceKey() noexcept;

private:
  void PrepareSize(const PixelRect &rc) noexcept;
  void OnResize(const PixelRect &rc);

  [[gnu::pure]]
  Button *FindButton(unsigned ch);

  void MoveButton(unsigned ch, PixelPoint position) noexcept;
  void ResizeButton(unsigned ch, PixelSize size) noexcept;
  void ResizeButtons();
  void MoveButtonsToRow(const PixelRect &rc, const char *row_keys, unsigned row,
                        int offset_left = 0) noexcept;
  void MoveButtons(const PixelRect &rc);

  [[gnu::pure]]
  static bool IsLandscape(const PixelRect &rc) {
    return rc.GetWidth() >= rc.GetHeight();
  }

  void UpdateShiftState() noexcept;
  void AddButton(ContainerWindow &parent, const char *caption, unsigned ch);
  void OnShiftClicked() noexcept;

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

  bool KeyPress(unsigned key_code) noexcept override {
    return KeyPressImpl(key_code, nullptr, nullptr);
  }

  /**
   * @param backspace optional backspace @c Button above the key grid.
   * @param action_row_first the first of the main row (usually @em OK);
   *  @c KEY_UP from @em OK focuses @em Space, then further @c KEY_UP
   *  follows the key grid to on-screen @em backspace; @c KEY_DOWN from
   *  the @em Space key focuses it; with no on-screen key below the
   *  focus, @c KEY_DOWN moves here too; @c KEY_UP from on-screen
   *  @em backspace also focuses it.
   */
  bool KeyPress(unsigned key_code, Button *backspace,
                Button *action_row_first = nullptr) noexcept {
    return KeyPressImpl(key_code, backspace, action_row_first);
  }

private:
  bool KeyPressImpl(unsigned key_code, Button *backspace,
                    Button *action_row_first) noexcept;

  bool RouteSpaceToActionRow(unsigned key_code, Button *action_row,
                            Window *w) noexcept;
  bool RouteNumberRowAndBackspace(unsigned key_code, Button *back,
                                  Window *w) noexcept;
  bool MoveFocusInGridByArrowKey(unsigned key_code, Window *w, Button *back,
                                 Button *action_row_first) noexcept;
  bool MoveByVerticalInGrid(int from, int diy, Button *back,
                            Button *action_row_first) noexcept;

  [[gnu::pure]]
  int GetNavigableCount() const noexcept {
    return (int)num_buttons + (show_shift_button ? 1 : 0);
  }

  const Button *GetByIndex(int idx) const noexcept;
  Button *GetByIndex(int idx) noexcept;

  bool TryNavigableKeyCenter(int from_idx, int j, PixelPoint &c_out) const
    noexcept;
  bool TrySetFocusToEnabledKey(int j) noexcept;

  [[gnu::pure]]
  int FindIndexOf(const Window *w) const noexcept;

  /** @return index of the @em Space key, or @a -1. */
  [[gnu::pure]]
  int GetSpaceKeyIndex() const noexcept;

  bool GetCenterByIndex(int idx, PixelPoint &out) const noexcept;

  [[gnu::pure]]
  int FindIndexVerticalFrom(int from_idx, int diy) const noexcept;
  [[gnu::pure]]
  int FindIndexHorizontalFrom(int from_idx, int dix) const noexcept;

  bool FocusFirstEnabledInNumberRow(int prefer_index) noexcept;
  bool FocusFirstEnabledInGrid() noexcept;
};
