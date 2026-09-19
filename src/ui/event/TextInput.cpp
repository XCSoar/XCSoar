// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextInput.hpp"
#include "ui/dim/Rect.hpp"

#ifdef ENABLE_SDL
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_stdinc.h>
#endif

namespace UI::TextInput {

bool
HasScreenKeyboard() noexcept
{
#ifdef ENABLE_SDL
  return ::SDL_HasScreenKeyboardSupport();
#else
  return false;
#endif
}

void
ShowScreenKeyboard() noexcept
{
#ifdef ENABLE_SDL
  /* only touch the text input state on platforms with an on-screen
     keyboard; on desktops, text input is enabled permanently and
     switching it off would break the physical keyboard */
  if (HasScreenKeyboard())
    if (auto *window = SDL_GetKeyboardFocus())
      ::SDL_StartTextInput(window);
#endif
}

void
HideScreenKeyboard() noexcept
{
#ifdef ENABLE_SDL
  if (HasScreenKeyboard())
    if (auto *window = SDL_GetKeyboardFocus())
      ::SDL_StopTextInput(window);
#endif
}

void
SetScreenKeyboardRect([[maybe_unused]] const PixelRect &rc) noexcept
{
#ifdef ENABLE_SDL
  if (!HasScreenKeyboard())
    return;

  SDL_Rect r{rc.left, rc.top, int(rc.GetWidth()), int(rc.GetHeight())};
  if (auto *window = SDL_GetKeyboardFocus())
    ::SDL_SetTextInputArea(window, &r, 0);
#endif
}

bool
HasClipboard() noexcept
{
#ifdef ENABLE_SDL
  return true;
#else
  return false;
#endif
}

std::string
GetClipboardText()
{
#ifdef ENABLE_SDL
  if (!::SDL_HasClipboardText())
    return {};

  char *text = ::SDL_GetClipboardText();
  if (text == nullptr)
    return {};

  std::string result{text};
  ::SDL_free(text);
  return result;
#else
  return {};
#endif
}

} // namespace UI::TextInput
