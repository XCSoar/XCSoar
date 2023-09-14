// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UtilsSystem.hpp"
#include "CommandLine.hpp"
#include "ui/dim/Size.hpp"

#ifdef ANDROID
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#endif

#include <tchar.h>

#ifdef _WIN32
#include <windef.h> // for HWND (needed by winuser.h)
#include <winuser.h>
#endif

#ifndef ANDROID

[[gnu::const]]
static PixelSize
GetWindowDecorationOverhead() noexcept
{
#ifdef _WIN32
  return {
    2 * GetSystemMetrics(SM_CXFIXEDFRAME),
    2 * GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION),
  };
#else
  return {};
#endif
}

#endif

/**
 * Returns the screen dimension rect to be used
 * @return The screen dimension rect to be used
 */
PixelSize
SystemWindowSize() noexcept
{
#ifdef ANDROID
  return native_view->GetSize();
#else
  /// @todo implement this properly for SDL/UNIX
  return PixelSize{ CommandLine::width, CommandLine::height } + GetWindowDecorationOverhead();
#endif
}
