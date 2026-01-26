// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/CharacterButton.hpp"
#include "util/CharUtil.hxx"
#include "util/UTF8.hpp"

#include <cassert>

void
CharacterButton::Create(ContainerWindow &parent, const ButtonLook &look,
                        const TCHAR *text, PixelRect rc,
                        OnCharacterCallback _on_character, unsigned _character,
                        const WindowStyle style) noexcept
{
  assert(_on_character);

  on_character = _on_character;
  character = _character;

  Button::Create(parent, look, text, rc, style);
}

unsigned
CharacterButton::GetUpperCharacter() const noexcept
{
  unsigned result = character;
  if (result < 0x80 && IsLowerAlphaASCII((TCHAR)result))
    result -= 'a' - 'A';
  return result;
}

void
CharacterButton::SetCharacter(unsigned _character) noexcept
{
  if (_character == character)
    return;

  character = _character;

  char buffer[7];
  *UnicodeToUTF8(character, buffer) = '\0';
  SetCaption(buffer);
}

bool
CharacterButton::OnClicked() noexcept
{
  on_character(character);
  return true;
}
