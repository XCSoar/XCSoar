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
