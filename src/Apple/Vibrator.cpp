// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

// Implementation of haptic feedback using UIKit's UIFeedbackGenerator
// Currently for iOS only, not macOS (UIFeedbackGenerator is iOS only)

#include "Vibrator.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#import <UIKit/UIKit.h>

namespace Apple {

bool
HaveHapticFeedback() noexcept
{
  /* assume that all iPhones have a "Taptic Engine" (which is true for
     all iPhones since the iPhone 7) and that iPads and iPods have
     none; asking CoreHaptics would be more precise, but generating
     feedback on a device without a "Taptic Engine" is a harmless
     no-op anyway */
  return UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPhone;
}

void
VibrateShort() noexcept
{
  /* keep the generator around; this keeps the "Taptic Engine" warmed
     up, reducing the latency of the next impulse */
  static UIImpactFeedbackGenerator *generator = nil;

  const dispatch_block_t generate = ^{
    if (generator == nil)
      generator = [[UIImpactFeedbackGenerator alloc]
                    initWithStyle:UIImpactFeedbackStyleLight];

    [generator impactOccurred];

    /* prepare the next impulse */
    [generator prepare];
  };

  /* UIKit may only be used on the main thread; call it directly when
     we are already there, so that the feedback is not deferred until
     the next run loop iteration (a modal dialog opened by the same
     event would delay it noticeably) */
  if ([NSThread isMainThread])
    generate();
  else
    dispatch_async(dispatch_get_main_queue(), generate);
}

} // namespace Apple

#endif // TARGET_OS_IPHONE
#endif // __APPLE__
