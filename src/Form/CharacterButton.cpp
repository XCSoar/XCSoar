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

#include "Form/CharacterButton.hpp"
#include "Look/DialogLook.hpp"

#ifndef _UNICODE
#include "Util/UTF8.hpp"
#endif

#include <assert.h>

void
CharacterButton::Create(ContainerWindow &parent, const DialogLook &look,
                        const TCHAR *text, PixelRect rc,
                        OnCharacterCallback _on_character, unsigned _character,
                        const ButtonWindowStyle style)
{
  assert(_on_character);

  on_character = _on_character;
  character = _character;

  ButtonWindow::Create(parent, text, rc, style);
  SetFont(*look.button.font);
}

void
CharacterButton::SetCharacter(unsigned _character)
{
  if (_character == character)
    return;

  character = _character;

#ifdef _UNICODE
  const TCHAR buffer[2] = { TCHAR(character), _T('\0') };
#else
  char buffer[7];
  *UnicodeToUTF8(character, buffer) = '\0';
#endif
  SetText(buffer);
}

bool
CharacterButton::OnClicked()
{
  on_character(character);
  return true;
}
