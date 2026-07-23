// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Asset.hpp"
#include "CommandLine.hpp"

#if (defined(USE_CONSOLE) && !defined(KOBO)) || defined(USE_WAYLAND)

bool
HasPointer() noexcept
{
  return true;
}

#endif

#if defined(USE_LIBINPUT) || defined(USE_WAYLAND)

bool
HasTouchScreen() noexcept
{
  return CommandLine::ApplyTouchInputOverride(
    IsAndroid() || IsKobo() || IsIOS());
}

bool
HasKeyboard() noexcept
{
  return !IsEmbedded();
}

#endif

bool
UseGreyscaleDisplay() noexcept
{
#if defined(GREYSCALE)
  return true;
#else
  return false;
#endif
}

bool
HasColors() noexcept
{
#if defined(GREYSCALE)
  return false;
#else
  return !IsKobo();
#endif
}

bool
IsDithered() noexcept
{
#ifdef DITHER
  return true;
#else
  return false;
#endif
}

bool
HasEPaper() noexcept
{
  return IsKobo();
}
