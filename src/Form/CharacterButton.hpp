// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Button.hpp"

#include <tchar.h>

/**
 * A button that emits a character on press.
 */
class CharacterButton : public Button {
  typedef bool (*OnCharacterCallback)(unsigned key);

  OnCharacterCallback on_character;
  unsigned character;

public:
  void Create(ContainerWindow &parent, const ButtonLook &look,
              const char *text, PixelRect rc,
              OnCharacterCallback on_character, unsigned character,
              const WindowStyle _style=WindowStyle()) noexcept;

  unsigned GetCharacter() const noexcept {
    return character;
  }

  /**
   * Convert GetCharacter() to upper case (ASCII only).
   */
  [[gnu::pure]]
  unsigned GetUpperCharacter() const noexcept;

  void SetCharacter(unsigned character) noexcept;

protected:
  /* virtual methods from class ButtonWindow */
  bool OnClicked() noexcept override;
};
