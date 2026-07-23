// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <TargetConditionals.h>

#if TARGET_OS_IPHONE

#import <UIKit/UIKit.h>

static inline CGFloat
GetUIScreenScale(UIScreen *screen) noexcept
{
#ifdef __IPHONE_8_0
  if ([screen respondsToSelector:@selector(nativeScale)])
    return screen.nativeScale;
#endif

  return screen.scale;
}

#endif
