// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <utility> // for std::pair

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

/** Preserve existing bindings without mapping punctuation to function keys. */
constexpr unsigned
TranslateBindingKeyCode(unsigned key_code) noexcept
{
  for (auto entry : key_code_translation_table)
    if (entry.from == key_code)
      return entry.to;

  // Linux reserves 249–255 below the button-code range.
  switch (key_code) {
  case KEY_SEMICOLON: return 249;
  case KEY_APOSTROPHE: return 250;
  default: return key_code;
  }
}

/**
 * @return the translated key code and a flag indicating whether this
 * is an (ASCII) character
 */
constexpr std::pair<unsigned, bool>
TranslateKeyCode(unsigned key_code, bool shift_down = false) noexcept
{
  if (shift_down) {
    switch (key_code) {
    case KEY_1: return {'!', true};
    case KEY_2: return {'@', true};
    case KEY_3: return {'#', true};
    case KEY_4: return {'$', true};
    case KEY_5: return {'%', true};
    case KEY_6: return {'^', true};
    case KEY_7: return {'&', true};
    case KEY_8: return {'*', true};
    case KEY_9: return {'(', true};
    case KEY_0: return {')', true};

    case KEY_MINUS: return {'_', true};
    case KEY_EQUAL: return {'+', true};
    case KEY_LEFTBRACE: return {'{', true};
    case KEY_RIGHTBRACE: return {'}', true};
    case KEY_BACKSLASH: return {'|', true};
    case KEY_SEMICOLON: return {':', true};
    case KEY_APOSTROPHE: return {'"', true};
    case KEY_GRAVE: return {'~', true};
    case KEY_COMMA: return {'<', true};
    case KEY_DOT: return {'>', true};
    case KEY_SLASH: return {'?', true};
    }
  }

  for (auto i : key_code_translation_table) {
    if (key_code == i.from) {
      return {i.to, i.is_char};
    }
  }

  switch (key_code) {
  case KEY_MINUS: return {'-', true};
  case KEY_EQUAL: return {'=', true};
  case KEY_LEFTBRACE: return {'[', true};
  case KEY_RIGHTBRACE: return {']', true};
  case KEY_BACKSLASH: return {'\\', true};
  case KEY_SEMICOLON: return {';', true};
  case KEY_APOSTROPHE: return {'\'', true};
  case KEY_GRAVE: return {'`', true};
  case KEY_COMMA: return {',', true};
  case KEY_DOT: return {'.', true};
  case KEY_SLASH: return {'/', true};
  }

  return {key_code, false};
}

/* these macros conflict with Event::Type */
#undef KEY_UP
#undef KEY_DOWN

/* wrong meaning */
#undef KEY_NEXT
