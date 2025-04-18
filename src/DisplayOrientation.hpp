// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class DisplayOrientation : uint8_t {
  DEFAULT,
  PORTRAIT,
  LANDSCAPE,
  REVERSE_PORTRAIT,
  REVERSE_LANDSCAPE,
};

#ifdef KOBO
/* Kobo defaults to portrait */
static constexpr DisplayOrientation DEFAULT_DISPLAY_ORIENTATION =
  DisplayOrientation::PORTRAIT;
#else
/* everything else defaults to landscape */
static constexpr DisplayOrientation DEFAULT_DISPLAY_ORIENTATION =
  DisplayOrientation::LANDSCAPE;
#endif

constexpr DisplayOrientation
TranslateDefaultDisplayOrientation(DisplayOrientation orientation)
{
  return orientation == DisplayOrientation::DEFAULT
    ? DEFAULT_DISPLAY_ORIENTATION
    : orientation;
}

constexpr bool
AreAxesSwapped(DisplayOrientation orientation)
{
  switch (TranslateDefaultDisplayOrientation(orientation)) {
  case DisplayOrientation::DEFAULT:
  case DisplayOrientation::LANDSCAPE:
  case DisplayOrientation::REVERSE_LANDSCAPE:
    break;

  case DisplayOrientation::PORTRAIT:
  case DisplayOrientation::REVERSE_PORTRAIT:
    return true;
  }

  return false;
}
