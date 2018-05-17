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

#ifndef XCSOAR_EVENT_LINUX_TRANSLATE_HPP
#define XCSOAR_EVENT_LINUX_TRANSLATE_HPP

#include "Asset.hpp"

#include <linux/input.h>

static constexpr struct {
  unsigned from, to;
  bool is_char;
} key_code_translation_table[] = {
#ifdef KOBO
  /* the Kobo Touch "home" button shall open the menu */
  { KEY_HOME, KEY_MENU, false },
#endif

  /* TODO: the ASCII codes overlap with many of the Linux key codes,
     and we should fix that */
  { KEY_0, '0', true },
  { KEY_1, '1', true },
  { KEY_2, '2', true },
  { KEY_3, '3', true },
  { KEY_4, '4', true },
  { KEY_5, '5', true },
  { KEY_6, '6', true },
  { KEY_7, '7', true },
  { KEY_8, '8', true },
  { KEY_9, '9', true },

  { KEY_A, 'A', true },
  { KEY_B, 'B', true },
  { KEY_C, 'C', true },
  { KEY_D, 'D', true },
  { KEY_E, 'E', true },
  { KEY_F, 'F', true },
  { KEY_G, 'G', true },
  { KEY_H, 'H', true },
  { KEY_I, 'I', true },
  { KEY_J, 'J', true },
  { KEY_K, 'K', true },
  { KEY_L, 'L', true },
  { KEY_M, 'M', true },
  { KEY_N, 'N', true },
  { KEY_O, 'O', true },
  { KEY_P, 'P', true },
  { KEY_Q, 'Q', true },
  { KEY_R, 'R', true },
  { KEY_S, 'S', true },
  { KEY_T, 'T', true },
  { KEY_U, 'U', true },
  { KEY_V, 'V', true },
  { KEY_W, 'W', true },
  { KEY_X, 'X', true },
  { KEY_Y, 'Y', true },
  { KEY_Z, 'Z', true },

  { KEY_SPACE, ' ', true },
};

gcc_const
static unsigned
TranslateKeyCode(unsigned key_code, bool &is_char)
{
  for (auto i : key_code_translation_table) {
    if (key_code == i.from) {
      is_char = i.is_char;
      return i.to;
    }
  }

  is_char = false;
  return key_code;
}

/* these macros conflict with Event::Type */
#undef KEY_UP
#undef KEY_DOWN

/* wrong meaning */
#undef KEY_NEXT

#endif
