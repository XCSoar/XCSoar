// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UtilsSystem.hpp"
#include "CommandLine.hpp"
#include "ui/dim/Size.hpp"

#ifdef ANDROID
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#endif

#ifdef _WIN32
#include <windef.h> // for HWND (needed by winuser.h)
#include <winuser.h>
#endif

#if defined(_WIN32) && defined(ENABLE_OPENGL)
#include "ui/display/sdl/Display.hpp"
#include <SDL_video.h>
#include <algorithm>
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif

#if !defined(ANDROID) && !defined(__APPLE__)

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
  
  // Check if window is null to prevent crashes
  if (!window) {
    // Fallback to main screen dimensions if no window is available
    CGRect screenBounds = [UIScreen mainScreen].bounds;
    CGFloat scale = [UIScreen mainScreen].nativeScale;
    return PixelSize{(int)(screenBounds.size.width * scale), (int)(screenBounds.size.height * scale)};
  }
  
  
  CGRect bounds = window.bounds;
  CGFloat scale = window.screen.nativeScale;

  CGFloat width = bounds.size.width;
  CGFloat height = bounds.size.height;

  int pixelWidth = (int)(width * scale);
  int pixelHeight = (int)(height * scale);

  return PixelSize{ pixelWidth, pixelHeight };
#elif defined(__APPLE__) && !TARGET_OS_IPHONE
  // Use default window size (SDL handles HiDPI scaling automatically)
  return PixelSize{ CommandLine::width, CommandLine::height };
#else
  /// @todo implement this properly for SDL/UNIX
#if defined(_WIN32) && defined(ENABLE_OPENGL)
  /* The OpenGL build declares per-monitor DPI awareness (via SDL), so Windows
   * no longer auto-scales the window.  Query the system DPI via SDL and scale
   * the requested window size so it appears at the intended physical size. */
  const auto dpi = SDL::Display::GetDPI();
  const auto overhead = GetWindowDecorationOverhead();
  PixelSize size = PixelSize{
    CommandLine::width * dpi.x / 96u,
    CommandLine::height * dpi.y / 96u,
  } + overhead;
  SDL_Rect usable;
  if (SDL_GetDisplayUsableBounds(0, &usable) == 0) {
    size.width = std::min(size.width, (unsigned)usable.w-overhead.width);
    size.height = std::min(size.height, (unsigned)usable.h-overhead.height);
  }
  return size;
#else
  return PixelSize{ CommandLine::width, CommandLine::height } + GetWindowDecorationOverhead();
#endif
#endif
}
