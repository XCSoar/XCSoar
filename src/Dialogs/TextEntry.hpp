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

#ifndef DIALOGS_TEXT_ENTRY_HPP
#define DIALOGS_TEXT_ENTRY_HPP

#include "Util/StringBuffer.hxx"

#include <functional>

#include <tchar.h>

typedef std::function<const TCHAR *(const TCHAR *)> AllowedCharacters;

bool
TextEntryDialog(TCHAR *text, size_t size,
                const TCHAR *caption=nullptr,
                AllowedCharacters ac=AllowedCharacters(),
                bool default_shift_state = true);

template<size_t N>
static inline bool
TextEntryDialog(StringBuffer<TCHAR, N> &text,
                const TCHAR *caption=NULL,
                AllowedCharacters accb=AllowedCharacters(),
                bool default_shift_state = true)
{
  return TextEntryDialog(text.data(), text.capacity(),
                         caption, accb, default_shift_state);
}

template<size_t N>
static inline bool
TextEntryDialog(StringBuffer<TCHAR, N> &text,
                const TCHAR *caption,
                bool default_shift_state)
{
  AllowedCharacters accb=AllowedCharacters();
  return TextEntryDialog(text.data(), text.capacity(),
                         caption, accb, default_shift_state);
}

void
KnobTextEntry(TCHAR *text, size_t width,
              const TCHAR *caption);

bool
TouchTextEntry(TCHAR *text, size_t size,
               const TCHAR *caption=nullptr,
               AllowedCharacters ac=AllowedCharacters(),
               bool default_shift_state = true);

#endif
