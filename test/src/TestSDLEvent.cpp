// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/event/sdl/Event.hpp"
#include "TestUtil.hpp"

int
main()
{
  plan_tests(8);

  UI::Event event;
  std::string input = "Aä中";
  event.event = {};
  event.event.type = SDL_EVENT_TEXT_INPUT;
  event.event.text.text = input.c_str();
  event.CopyText();

  /* Simulate SDL releasing its text during a nested event loop. */
  input.clear();
  ok1(event.GetCharacterCount() == 3);
  ok1(event.GetCharacter(0) == 'A');
  ok1(event.GetCharacter(1) == 0xe4);
  ok1(event.GetCharacter(2) == 0x4e2d);

  const UI::Event copy = event;
  event.text.clear();
  ok1(copy.GetCharacterCount() == 3);
  ok1(copy.GetCharacter(2) == 0x4e2d);

  event.event.type = SDL_EVENT_KEY_DOWN;
  event.event.key.key = SDLK_F1;
  ok1(event.GetCharacterCount() == 0);
  ok1(event.GetKeyCode() == SDLK_F1);

  return exit_status();
}
