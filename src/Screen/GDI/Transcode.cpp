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

#include "Transcode.hpp"
#include "Key.h"
#include "Asset.hpp"

struct KeyMap {
  unsigned from, to;
};

static constexpr KeyMap altair_key_map[] = {
  { VK_F1, VK_APP1 },
  { VK_F2, VK_APP2 },
  { VK_F3, VK_APP3 },
  { VK_F4, VK_APP4 },
  { VK_F5, '6' },
  { VK_F6, '7' },
  { VK_F7, '8' },
  { VK_F8, '9' },
  { VK_F9, '0' },
  { 0 }
};

static constexpr KeyMap hp31x_key_map[] = {
  { VK_F12, VK_ESCAPE },
  { 0 }
};

static constexpr KeyMap medion_p5_key_map[] = {
  { VK_F6, 0x26 }, // + key = pg up
  { VK_F7, 0x28 }, // - key = pg down
  { VK_F8, VK_RETURN }, // middle key = enter
  { 0 }
};

static constexpr KeyMap nokia_500_key_map[] = {
  { 0xc1, VK_RETURN }, // middle key = enter
  { 0xc5, 0x26 }, // + key = pg down
  { 0xc6, 0x28 }, // - key = pg up
  { 0 }
};

static constexpr KeyMap pn_6000_key_map[] = {
  { VK_F10, VK_APP1 }, // Upper Silver key short press
  { VK_F12, VK_APP2 }, // Lower Silver key short press
  { VK_F3, VK_APP3 }, // Back key plus
  { VK_F2, VK_APP4 }, // Back key minus
  { VK_F11, VK_F1 }, // Upper silver key LONG press
  { VK_F13, VK_F2 }, // Lower silver key LONG press
  { 0 }
};

static constexpr KeyMap lx_mm_key_map[] = {
  { 'L', VK_APP1 }, // NAV
  { 'N', VK_APP2 }, // TSK/TRG
  { 'C', VK_APP3 }, // SET/SYS
  { 'P', VK_APP4 }, // INFO
  { 'F', VK_F1 }, // AN/CLC (long press)
  { 'H', VK_F2 }, // START/R (long press)
  { 'R', VK_F3 }, // INFO (long press)
  { 'M', VK_F4 }, // NAV (long press)
  { 'E', VK_F5 }, // AN/CLC
  { 'G', VK_F6 }, // START/R
  { 'O', VK_F7 }, // TSK/TRG (long press)
  { 'I', VK_F8 }, // SET/SYS (long press)
  { ' ', VK_MENU }, // press rotary knop (left buttom)
  { VK_UP, VK_DOWN }, // Invert direction of rotation
  { VK_DOWN, VK_UP }, // of the UP/DOWN rotary knob
  { 0 }
};

gcc_const
static unsigned
KeyMapLookup(const KeyMap *map, unsigned key_code)
{
  for (auto i = map; i->from != 0; ++i)
    if (i->from == key_code)
      return i->to;

  return key_code;
}

unsigned
TranscodeKey(unsigned key_code)
{
  if (IsAltair())
    return KeyMapLookup(altair_key_map, key_code);

#ifdef _WIN32_WCE
  /* VK_F23 is the "action" key on some iPaqs */
  static bool seen_return = false;
  if (key_code == VK_RETURN)
    /* some devices send both VK_F23 and VK_RETURN; don't translate
       VK_F23 to VK_RETURN if get a "real" VK_RETURN message */
    seen_return = true;
  else if (key_code == VK_F23 && !seen_return)
    return VK_RETURN;
#endif

  switch (global_model_type) {
  case ModelType::HP31X:
    return KeyMapLookup(hp31x_key_map, key_code);

  case ModelType::MEDION_P5:
    return KeyMapLookup(medion_p5_key_map, key_code);

  case ModelType::NOKIA_500:
    return KeyMapLookup(nokia_500_key_map, key_code);

  case ModelType::PN6000:
    return KeyMapLookup(pn_6000_key_map, key_code);

  case ModelType::LX_MINI_MAP:
    return KeyMapLookup(lx_mm_key_map, key_code);

  default:
    return key_code;
  }
}
