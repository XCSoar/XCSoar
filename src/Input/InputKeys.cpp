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
  { "APP1", KEY_APP1 },
  { "APP2", KEY_APP2 },
  { "APP3", KEY_APP3 },
  { "APP4", KEY_APP4 },
  { "APP5", KEY_APP5 },
  { "APP6", KEY_APP6 },
  { "F1", KEY_F1 },
  { "F2", KEY_F2 },
  { "F3", KEY_F3 },
  { "F4", KEY_F4 },
  { "F5", KEY_F5 },
  { "F6", KEY_F6 },
  { "F7", KEY_F7 },
  { "F8", KEY_F8 },
  { "F9", KEY_F9 },
  { "F10", KEY_F10 },
  { "F11", KEY_F11 },
  { "F12", KEY_F12 },

#ifdef ANDROID
  /* These keys are used by BlueTooth keypads and available in Android*/
  { "BUTTON_R1", KEYCODE_BUTTON_R1},
  { "BUTTON_R2", KEYCODE_BUTTON_R2},
  { "BUTTON_L1", KEYCODE_BUTTON_L1},
  { "BUTTON_L2", KEYCODE_BUTTON_L2},
  { "BUTTON_A", KEYCODE_BUTTON_A},
  { "BUTTON_B", KEYCODE_BUTTON_B},
  { "BUTTON_C", KEYCODE_BUTTON_C},
  { "BUTTON_X", KEYCODE_BUTTON_X},
  { "BUTTON_Y", KEYCODE_BUTTON_Y},
  { "BUTTON_Z", KEYCODE_BUTTON_Z},
  { "MEDIA_NEXT", KEYCODE_MEDIA_NEXT},
  { "MEDIA_PREVIOUS", KEYCODE_MEDIA_PREVIOUS},
  { "MEDIA_PLAY_PAUSE", KEYCODE_MEDIA_PLAY_PAUSE},
  { "VOLUME_UP", KEY_VOLUME_UP },
  { "VOLUME_DOWN", KEY_VOLUME_DOWN },  
#endif

#ifdef USE_WINUSER
  /* These Keys are used for the Triadis-RemoteStick, as well as for
     expanded Keyboard-Events */
  { "F13", KEY_F13 },
  { "F14", KEY_F14 },
  { "F15", KEY_F15 },
  { "F16", KEY_F16 },
  { "F17", KEY_F17 },
  { "F18", KEY_F18 },
  { "F19", KEY_F19 },
  { "F20", KEY_F20 },
#endif
  { "LEFT", KEY_LEFT },
  { "RIGHT", KEY_RIGHT },
  { "UP", KEY_UP },
  { "DOWN", KEY_DOWN },
  { "RETURN", KEY_RETURN },
  { "ESCAPE", KEY_ESCAPE },
  { "MENU", KEY_MENU },
  { "TAB", KEY_TAB },
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
