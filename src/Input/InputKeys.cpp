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

#include "InputKeys.hpp"
#include "Screen/Key.h"

#include <string.h>
#include <ctype.h>

struct string_to_key {
  const TCHAR *name;
  unsigned key;
};

static constexpr struct string_to_key string_to_key[] = {
  { _T("APP1"), VK_APP1 },
  { _T("APP2"), VK_APP2 },
  { _T("APP3"), VK_APP3 },
  { _T("APP4"), VK_APP4 },
  { _T("APP5"), VK_APP5 },
  { _T("APP6"), VK_APP6 },
  { _T("F1"), VK_F1 },
  { _T("F2"), VK_F2 },
  { _T("F3"), VK_F3 },
  { _T("F4"), VK_F4 },
  { _T("F5"), VK_F5 },
  { _T("F6"), VK_F6 },
  { _T("F7"), VK_F7 },
  { _T("F8"), VK_F8 },
  { _T("F9"), VK_F9 },
  { _T("F10"), VK_F10 },
  { _T("F11"), VK_F11 },
  { _T("F12"), VK_F12 },
  { _T("LEFT"), VK_LEFT },
  { _T("RIGHT"), VK_RIGHT },
  { _T("UP"), VK_UP },
  { _T("DOWN"), VK_DOWN },
  { _T("RETURN"), VK_RETURN },
  { _T("ESCAPE"), VK_ESCAPE },
  { _T("MENU"), VK_MENU },
  { NULL }
};

unsigned
ParseKeyCode(const TCHAR *data)
{
  for (const struct string_to_key *p = &string_to_key[0]; p->name != NULL; ++p)
    if (_tcscmp(data, p->name) == 0)
      return p->key;

  if (_tcslen(data) == 1)
    return _totupper(data[0]);

  else
    return 0;

}
