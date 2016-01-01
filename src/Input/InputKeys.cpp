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

#include "InputKeys.hpp"
#include "Event/KeyCode.hpp"
#include "Util/CharUtil.hpp"
#include "Util/StringAPI.hxx"

struct string_to_key {
  const TCHAR *name;
  unsigned key;
};

static constexpr struct string_to_key string_to_key[] = {
  { _T("APP1"), KEY_APP1 },
  { _T("APP2"), KEY_APP2 },
  { _T("APP3"), KEY_APP3 },
  { _T("APP4"), KEY_APP4 },
  { _T("APP5"), KEY_APP5 },
  { _T("APP6"), KEY_APP6 },
  { _T("F1"), KEY_F1 },
  { _T("F2"), KEY_F2 },
  { _T("F3"), KEY_F3 },
  { _T("F4"), KEY_F4 },
  { _T("F5"), KEY_F5 },
  { _T("F6"), KEY_F6 },
  { _T("F7"), KEY_F7 },
  { _T("F8"), KEY_F8 },
  { _T("F9"), KEY_F9 },
  { _T("F10"), KEY_F10 },
  { _T("F11"), KEY_F11 },
  { _T("F12"), KEY_F12 },
#ifdef USE_WINUSER
  /* These Keys are used for the Triadis-RemoteStick, as well as for
     expanded Keyboard-Events */
  { _T("F13"), KEY_F13 },
  { _T("F14"), KEY_F14 },
  { _T("F15"), KEY_F15 },
  { _T("F16"), KEY_F16 },
  { _T("F17"), KEY_F17 },
  { _T("F18"), KEY_F18 },
  { _T("F19"), KEY_F19 },
  { _T("F20"), KEY_F20 },
#endif
  { _T("LEFT"), KEY_LEFT },
  { _T("RIGHT"), KEY_RIGHT },
  { _T("UP"), KEY_UP },
  { _T("DOWN"), KEY_DOWN },
  { _T("RETURN"), KEY_RETURN },
  { _T("ESCAPE"), KEY_ESCAPE },
  { _T("MENU"), KEY_MENU },
  { NULL }
};

unsigned
ParseKeyCode(const TCHAR *data)
{
  for (const struct string_to_key *p = &string_to_key[0]; p->name != NULL; ++p)
    if (StringIsEqual(data, p->name))
      return p->key;

  if (StringLength(data) == 1)
    return ToUpperASCII(data[0]);

  else
    return 0;

}
