// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyboardDetection.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#import <GameController/GCKeyboard.h>

[[nodiscard]]
bool
IsHardwareKeyboardConnected() noexcept
{
  if (@available(iOS 14.0, *))
    return GCKeyboard.coalescedKeyboard != nil;

  return false;
}

#endif // TARGET_OS_IPHONE
#endif // __APPLE__
