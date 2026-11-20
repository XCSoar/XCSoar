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

int
main()
{
  plan_tests(7);
  TestKeyEvents();
  return exit_status();
}
