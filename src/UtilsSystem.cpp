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

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif

#if !defined(ANDROID) && !(defined(__APPLE__) && TARGET_OS_IPHONE)

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
#elif defined(__APPLE__) && TARGET_OS_IPHONE
  UIWindow *window = UIApplication.sharedApplication.windows.firstObject;
  CGRect bounds = window.bounds;
  CGFloat scale = window.screen.scale;

  CGFloat width = bounds.size.width;
  CGFloat height = bounds.size.height;

  int pixelWidth = (int)(width * scale);
  int pixelHeight = (int)(height * scale);

  return PixelSize{ pixelWidth, pixelHeight };
#else
  /// @todo implement this properly for SDL/UNIX
  return PixelSize{ CommandLine::width, CommandLine::height } + GetWindowDecorationOverhead();
#endif
}
