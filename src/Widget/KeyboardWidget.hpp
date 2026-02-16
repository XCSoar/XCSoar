// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget.hpp"
#include "Form/CharacterButton.hpp"
#include "Form/Button.hpp"

#include <tchar.h>

struct ButtonLook;
class WndSymbolButton;

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

private:
  void PrepareSize(const PixelRect &rc);
  void OnResize(const PixelRect &rc);

  [[gnu::pure]]
  Button *FindButton(unsigned ch);

  void MoveButton(unsigned ch, PixelPoint position) noexcept;
  void ResizeButton(unsigned ch, PixelSize size) noexcept;
  void ResizeButtons();
  void SetButtonsSize();
  void MoveButtonsToRow(const PixelRect &rc,
                        const char *buttons, unsigned row,
                        int offset_left = 0);
  void MoveButtons(const PixelRect &rc);

  [[gnu::pure]]
  static bool IsLandscape(const PixelRect &rc) {
    return rc.GetWidth() >= rc.GetHeight();
  }

  /* updates UI based on value of shift_state property */
  void UpdateShiftState();

  void AddButton(ContainerWindow &parent, const char *caption, unsigned ch);

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;

private:
  void OnShiftClicked();
};
