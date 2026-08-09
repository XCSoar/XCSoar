// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Asset.hpp"
#include "CommandLine.hpp"
#include "Hardware/CPU.hpp"

#ifndef KOBO
static DisplayType display_type = DisplayType::LCD;
#else
static DisplayType display_type = DisplayType::E_INK;
#endif

void
SetDisplayType(DisplayType type) noexcept
{
  display_type = type;
}

bool
HasEPaper() noexcept
{
  return IsEPaperDisplayType(display_type);
}

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

/**
 * Default for harness / tools that link Form/List without
 * Hardware/CPU.cpp.  The strong definition in CPU.cpp overrides this
 * in the main binary.
 */
[[gnu::weak]]
bool
IsSlowCPU() noexcept
{
  return false;
}