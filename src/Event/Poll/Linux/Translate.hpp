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

#ifndef XCSOAR_EVENT_LINUX_TRANSLATE_HPP
#define XCSOAR_EVENT_LINUX_TRANSLATE_HPP

#include "Asset.hpp"

#include <linux/input.h>

static constexpr struct {
  unsigned from, to;
} key_code_translation_table[] = {
#ifdef KOBO
  /* the Kobo Touch "home" button shall open the menu */
  { KEY_HOME, KEY_MENU },
#endif

  /* TODO: the ASCII codes overlap with many of the Linux key codes,
     and we should fix that */
  { KEY_0, '0' },
  { KEY_1, '1' },
  { KEY_2, '2' },
  { KEY_3, '3' },
  { KEY_4, '4' },
  { KEY_5, '5' },
  { KEY_6, '6' },
  { KEY_7, '7' },
  { KEY_8, '8' },
  { KEY_9, '9' },

  { KEY_A, 'A' },
  { KEY_B, 'B' },
  { KEY_C, 'C' },
  { KEY_D, 'D' },
  { KEY_E, 'E' },
  { KEY_F, 'F' },
  { KEY_G, 'G' },
  { KEY_H, 'H' },
  { KEY_I, 'I' },
  { KEY_J, 'J' },
  { KEY_K, 'K' },
  { KEY_L, 'L' },
  { KEY_M, 'M' },
  { KEY_N, 'N' },
  { KEY_O, 'O' },
  { KEY_P, 'P' },
  { KEY_Q, 'Q' },
  { KEY_R, 'R' },
  { KEY_S, 'S' },
  { KEY_T, 'T' },
  { KEY_U, 'U' },
  { KEY_V, 'V' },
  { KEY_W, 'W' },
  { KEY_X, 'X' },
  { KEY_Y, 'Y' },
  { KEY_Z, 'Z' },
};

gcc_const
static unsigned
TranslateKeyCode(unsigned key_code)
{
  for (auto i : key_code_translation_table)
    if (key_code == i.from)
      return i.to;

  return key_code;
}

/* these macros conflict with Event::Type */
#undef KEY_UP
#undef KEY_DOWN

/* wrong meaning */
#undef KEY_NEXT

#endif
