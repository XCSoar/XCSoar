// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Asset.hpp"
#include "CommandLine.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/DitherPass.hpp"
#endif

#if defined(USE_CONSOLE) || defined(USE_WAYLAND)
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

#if (defined(USE_CONSOLE) && !defined(KOBO)) || defined(USE_WAYLAND)

bool
HasPointer() noexcept
{
  return UI::event_queue->HasPointer();
}

#endif

#if defined(USE_LIBINPUT) || defined(USE_WAYLAND)

static bool
HasTouchScreenHardware() noexcept
{
  return UI::event_queue->HasTouchScreen();
}

bool
HasTouchScreen() noexcept
{
  return CommandLine::ApplyTouchInputOverride(HasTouchScreenHardware());
}

bool
HasKeyboard() noexcept
{
  return UI::event_queue->HasKeyboard();
}

#endif /* USE_LIBINPUT || USE_WAYLAND */

bool
UseGreyscaleDisplay() noexcept
{
#if defined(GREYSCALE)
  return true;
#elif defined(ENABLE_OPENGL)
  return OpenGL::enable_dither_pass;
#else
  return false;
#endif
}

bool
HasColors() noexcept
{
#if defined(GREYSCALE)
  return false;
#elif defined(ENABLE_OPENGL)
  if (OpenGL::enable_dither_pass)
    return false;
  return !IsKobo();
#else
  return !IsKobo();
#endif
}

bool
IsDithered() noexcept
{
#ifdef DITHER
  return true;
#elif defined(ENABLE_OPENGL)
  return OpenGL::enable_dither_pass;
#else
  return false;
#endif
}

bool
HasEPaper() noexcept
{
  if (IsKobo())
    return true;

#ifdef ENABLE_OPENGL
  return OpenGL::enable_dither_pass;
#else
  return false;
#endif
}
