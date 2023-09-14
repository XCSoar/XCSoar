// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Asset.hpp"

#if (defined(USE_CONSOLE) && !defined(KOBO)) || defined(USE_WAYLAND)

bool
HasPointer() noexcept
{
  return true;
}

#endif

#ifdef USE_LIBINPUT

bool
HasTouchScreen() noexcept
{
  return IsAndroid() || IsKobo() || IsIOS();
}

bool
HasKeyboard() noexcept
{
  return !IsEmbedded();
}

#endif
