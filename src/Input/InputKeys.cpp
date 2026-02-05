// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputKeys.hpp"
#include "ui/event/KeyCode.hpp"
#include "util/CharUtil.hxx"
#include "util/StringAPI.hxx"

struct string_to_key {
  const char *name;
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

#ifdef ANDROID
  /* These keys are used by BlueTooth keypads and available in Android*/
  { _T("BUTTON_R1"), KEYCODE_BUTTON_R1},
  { _T("BUTTON_R2"), KEYCODE_BUTTON_R2},
  { _T("BUTTON_L1"), KEYCODE_BUTTON_L1},
  { _T("BUTTON_L2"), KEYCODE_BUTTON_L2},
  { _T("BUTTON_A"), KEYCODE_BUTTON_A},
  { _T("BUTTON_B"), KEYCODE_BUTTON_B},
  { _T("BUTTON_C"), KEYCODE_BUTTON_C},
  { _T("BUTTON_X"), KEYCODE_BUTTON_X},
  { _T("BUTTON_Y"), KEYCODE_BUTTON_Y},
  { _T("BUTTON_Z"), KEYCODE_BUTTON_Z},
  { _T("MEDIA_NEXT"), KEYCODE_MEDIA_NEXT},
  { _T("MEDIA_PREVIOUS"), KEYCODE_MEDIA_PREVIOUS},
  { _T("MEDIA_PLAY_PAUSE"), KEYCODE_MEDIA_PLAY_PAUSE},
  { _T("VOLUME_UP"), KEY_VOLUME_UP },
  { _T("VOLUME_DOWN"), KEY_VOLUME_DOWN },  
#endif

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
  { _T("TAB"), KEY_TAB },
  { NULL }
};

unsigned
ParseKeyCode(const char *data)
{
  for (const struct string_to_key *p = &string_to_key[0]; p->name != NULL; ++p)
    if (StringIsEqual(data, p->name))
      return p->key;

  if (StringLength(data) == 1)
    return ToUpperASCII(data[0]);

  else
    return 0;

}
