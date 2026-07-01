// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyboardDetection.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#ifdef __IPHONE_14_0
#import <GameController/GCKeyboard.h>
#endif

[[nodiscard]]
bool
IsHardwareKeyboardConnected() noexcept
{
#ifdef __IPHONE_14_0
  if (@available(iOS 14.0, *))
    return GCKeyboard.coalescedKeyboard != nil;
#endif

  return false;
}

#endif // TARGET_OS_IPHONE
#endif // __APPLE__
