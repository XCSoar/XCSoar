// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/event/shared/Event.hpp"
#include "util/StringCompare.hxx"
#include "util/UTF8.hpp"
#include "TestUtil.hpp"

#ifdef TEST_LINUX_KEYBOARD_STATE
#include "ui/event/poll/linux/KeyboardState.hpp"
#include "ui/event/poll/linux/Translate.hpp"
#include <linux/input.h>
#include <linux/keyboard.h>

/* Linux key macros conflict with the shared event type names. */
#undef KEY_UP
#undef KEY_DOWN
#endif

#include <string>

static void
TestKeyEvents()
{
  UI::Event key(UI::Event::KEY_DOWN, 103);
  ok1(key.GetCharacterCount() == 0);
  key.ch = 'g';
  ok1(key.GetCharacterCount() == 1);
  ok1(key.GetCharacter(0) == 'g');
  ok1(key.GetKeyCode() == 103);
  key.ch = ';';
  ok1(key.GetCharacter(0) == ';');
  key.type = UI::Event::KEY_UP;
  ok1(key.GetCharacterCount() == 0);
  ok1(UI::Event(UI::Event::KEY_DOWN, 28).GetCharacterCount() == 0);
}

#ifdef TEST_LINUX_KEYBOARD_STATE
static void
TestKeyboardState()
{
  UI::KeyboardState keyboard;
  keyboard.Update(KEY_LEFTCTRL, true);
  ok1(keyboard.GetCharacter(KEY_SEMICOLON) == 0);
  ok1(TranslateBindingKeyCode(KEY_SEMICOLON) != KEY_F1);
  keyboard.Update(KEY_LEFTCTRL, false);
  ok1(keyboard.GetCharacter(KEY_SEMICOLON) == ';');
  ok1(keyboard.GetCharacter(KEY_KP1) == 0);
  keyboard.Update(KEY_NUMLOCK, true);
  keyboard.Update(KEY_NUMLOCK, true);
  ok1(keyboard.GetCharacter(KEY_KP1) == '1');
  ok1(keyboard.GetCharacter(KEY_KPPLUS) == '+');
  ok1(keyboard.GetCharacter(KEY_KPDOT) == '.');
  keyboard.Reset();
  ok1(keyboard.GetCharacter(KEY_KP1) == 0);
  ok1(keyboard.GetCharacter(KEY_A) == 'a');
  keyboard.Update(KEY_LEFTSHIFT, true);
  ok1(keyboard.GetCharacter(KEY_A) == 'A');
  keyboard.Update(KEY_RIGHTSHIFT, true);
  keyboard.Update(KEY_LEFTSHIFT, false);
  ok1(keyboard.GetCharacter(KEY_1) == '!');
  keyboard.Update(KEY_RIGHTSHIFT, false);
  ok1(keyboard.GetCharacter(KEY_1) == '1');
  keyboard.Update(KEY_CAPSLOCK, true);
  keyboard.Update(KEY_CAPSLOCK, true); // repeat
  ok1(keyboard.GetCharacter(KEY_A) == 'A');
  keyboard.Update(KEY_CAPSLOCK, false);
  ok1(keyboard.GetCharacter(KEY_A) == 'A');
  keyboard.Update(KEY_LEFTSHIFT, true);
  ok1(keyboard.GetCharacter(KEY_A) == 'a');
  ok1(keyboard.GetCharacter(KEY_1) == '!');
  keyboard.Reset();
  ok1(keyboard.GetCharacter(KEY_A) == 'a');
  ok1(keyboard.GetCharacter(KEY_G) == 'g');
  ok1(keyboard.GetCharacter(KEY_I) == 'i');
  ok1(keyboard.GetCharacter(KEY_J) == 'j');
  ok1(keyboard.GetCharacter(KEY_L) == 'l');
  ok1(keyboard.GetCharacter(KEY_SPACE) == ' ');
  ok1(keyboard.GetCharacter(KEY_ENTER) < 0x20);
  keyboard.Update(KEY_LEFTSHIFT, true);
  keyboard.Update(KEY_LEFTSHIFT, true); // repeat
  keyboard.Update(KEY_LEFTSHIFT, false);
  ok1(keyboard.GetCharacter(KEY_1) == '1');
  keyboard.Update(KEY_LEFTSHIFT, true);
  keyboard.Reset(); // disconnect/suspend
  ok1(keyboard.GetCharacter(KEY_1) == '1');
  keyboard.Update(0xffffffff, true);
  ok1(keyboard.GetCharacter(0xffffffff) == 0);
  keyboard.Update(KEY_LEFTCTRL, true);
  ok1(keyboard.GetCharacter(KEY_A) < 0x20);
  keyboard.Reset();

  static constexpr struct { unsigned key; char plain, shifted; } symbols[] = {
    {KEY_MINUS, '-', '_'}, {KEY_EQUAL, '=', '+'},
    {KEY_LEFTBRACE, '[', '{'}, {KEY_RIGHTBRACE, ']', '}'},
    {KEY_BACKSLASH, '\\', '|'}, {KEY_SEMICOLON, ';', ':'},
    {KEY_APOSTROPHE, '\'', '"'}, {KEY_GRAVE, '`', '~'},
    {KEY_COMMA, ',', '<'}, {KEY_DOT, '.', '>'}, {KEY_SLASH, '/', '?'},
  };
  for (const auto &symbol : symbols) {
    ok1(keyboard.GetCharacter(symbol.key) == unsigned(symbol.plain));
    keyboard.Update(KEY_LEFTSHIFT, true);
    ok1(keyboard.GetCharacter(symbol.key) == unsigned(symbol.shifted));
    keyboard.Update(KEY_LEFTSHIFT, false);
  }

  UI::KeyboardState::Keymap map{};
  map[0][KEY_Z] = K(KT_LETTER, 'y');
  map[1][KEY_Z] = K(KT_LETTER, 'Y');
  map[0][KEY_Y] = K(KT_LETTER, 'z');
  map[0][KEY_Q] = K(KT_LETTER, 'q');
  map[2][KEY_Q] = K(KT_LATIN, '@');
  map[2][KEY_7] = K(KT_LATIN, '{');
  map[2][KEY_8] = K(KT_LATIN, '[');
  map[2][KEY_9] = K(KT_LATIN, ']');
  map[2][KEY_0] = K(KT_LATIN, '}');
  map[3][KEY_Q] = K(KT_LATIN, '~');
  map[0][KEY_KP1] = K_P1;
  map[0][KEY_KPPLUS] = K_PPLUS;
  map[0][KEY_KPDOT] = K_PDOT;
  UI::KeyboardState german(&map);
  ok1(german.GetCharacter(KEY_KP1) == 0);
  german.Update(KEY_NUMLOCK, true);
  ok1(german.GetCharacter(KEY_KP1) == '1');
  ok1(german.GetCharacter(KEY_KPPLUS) == '+');
  ok1(german.GetCharacter(KEY_KPDOT) == '.');
  german.Update(KEY_NUMLOCK, false);
  german.Update(KEY_NUMLOCK, true);
  ok1(german.GetCharacter(KEY_KP1) == 0);
  ok1(german.GetCharacter(KEY_KPPLUS) == '+');
  ok1(german.GetCharacter(KEY_KPDOT) == 0);
  ok1(german.GetCharacter(KEY_Z) == 'y');
  ok1(german.GetCharacter(KEY_Y) == 'z');
  german.Update(KEY_RIGHTALT, true);
  ok1(german.GetCharacter(KEY_Q) == '@');
  ok1(german.GetCharacter(KEY_7) == '{');
  ok1(german.GetCharacter(KEY_8) == '[');
  ok1(german.GetCharacter(KEY_9) == ']');
  ok1(german.GetCharacter(KEY_0) == '}');
  german.Update(KEY_RIGHTALT, false);
  ok1(german.GetCharacter(KEY_Q) == 'q');
  german.Update(KEY_LEFTSHIFT, true);
  ok1(german.GetCharacter(KEY_Z) == 'Y');
  german.Update(KEY_CAPSLOCK, true);
  ok1(german.GetCharacter(KEY_Z) == 'y');
  german.Update(KEY_LEFTSHIFT, false);
  ok1(german.GetCharacter(KEY_Z) == 'Y');
  german.Reset();
  german.Update(KEY_LEFTSHIFT, true);
  german.Update(KEY_RIGHTALT, true);
  ok1(german.GetCharacter(KEY_Q) == '~');
  german.Reset();
  ok1(german.GetCharacter(KEY_Z) == 'y');
  german.Update(KEY_LEFTCTRL, true);
  ok1(german.GetCharacter(KEY_Q) == 0);
  german.Reset();
  ok1(german.GetCharacter(256) == 0);
  ok1(german.GetCharacter(KEY_F1) == 0);
  map[0][KEY_A] = K(KT_DEAD, 0);
  map[0][KEY_E] = K(KT_LETTER, 0xe9);
  UI::KeyboardState restricted(&map);
  ok1(restricted.GetCharacter(KEY_A) == 0);
  ok1(restricted.GetCharacter(KEY_E) == 0);
  UI::KeyboardState copied(german.GetKeymap());
  ok1(copied.GetCharacter(KEY_Z) == 'y');
  UI::KeyboardState fallback(nullptr);
  ok1(fallback.GetCharacter(KEY_Z) == 'z');
  fallback.Update(KEY_RIGHTALT, true);
  ok1(fallback.GetCharacter(KEY_Q) == 0);
  fallback.Update(KEY_RIGHTALT, false);
  ok1(fallback.GetCharacter(KEY_Q) == 'q');

  // Representative console entries; no layout names are hard-coded in input.
  for (const auto &entry : {
      std::array<unsigned short, 3>{KEY_Q, 'a', 'A'}, // French AZERTY
      {KEY_2, '2', '"'}, // UK
      {KEY_Z, 'z', 'Z'}, // Polish QWERTY
      {KEY_Z, 'y', 'Y'}, // Czech QWERTZ
    }) {
    map[0][entry[0]] = K(KT_LATIN, entry[1]);
    map[1][entry[0]] = K(KT_LATIN, entry[2]);
    UI::KeyboardState layout(&map);
    ok1(layout.GetCharacter(entry[0]) == entry[1]);
    layout.Update(KEY_LEFTSHIFT, true);
    ok1(layout.GetCharacter(entry[0]) == entry[2]);
  }
}
#endif

int
main()
{
  plan_tests(7
#ifdef TEST_LINUX_KEYBOARD_STATE
             + 19 + 22 + 8 + 14 + 8 + 9 + 6
#endif
             );
  TestKeyEvents();
#ifdef TEST_LINUX_KEYBOARD_STATE
  TestKeyboardState();
#endif
  return exit_status();
}
