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
Vibrate(HapticFeedbackType type) noexcept
{
  /* keep the generators around; this keeps the "Taptic Engine" warmed
     up, reducing the latency of the next impulse */
  static UIImpactFeedbackGenerator *press_generator = nil, *impact_generator = nil;
  static UINotificationFeedbackGenerator *notification_generator = nil;
  static UISelectionFeedbackGenerator *selection_generator = nil;

  const dispatch_block_t generate = ^{
    switch (type) {
    case HapticFeedbackType::SELECTION:
      if (selection_generator == nil)
        selection_generator = [[UISelectionFeedbackGenerator alloc] init];

      [selection_generator selectionChanged];
      [selection_generator prepare];
      break;

    case HapticFeedbackType::PRESS:
      if (press_generator == nil)
        press_generator = [[UIImpactFeedbackGenerator alloc]
                            initWithStyle:UIImpactFeedbackStyleLight];

      [press_generator impactOccurred];
      [press_generator prepare];
      break;

    case HapticFeedbackType::LONG_PRESS:
    case HapticFeedbackType::GESTURE:
      if (impact_generator == nil)
        impact_generator = [[UIImpactFeedbackGenerator alloc]
                             initWithStyle:UIImpactFeedbackStyleMedium];

      [impact_generator impactOccurred];
      [impact_generator prepare];
      break;

    case HapticFeedbackType::NOTIFICATION:
      if (notification_generator == nil)
        notification_generator = [[UINotificationFeedbackGenerator alloc] init];

      /* the "warning" pattern is the one iOS uses for a message that
         asks for the user's attention */
      [notification_generator
        notificationOccurred:UINotificationFeedbackTypeWarning];
      [notification_generator prepare];
      break;

    case HapticFeedbackType::ALARM:
      if (notification_generator == nil)
        notification_generator = [[UINotificationFeedbackGenerator alloc] init];

      /* the "error" pattern is the most insistent one iOS offers, and
         it is distinct from the one used for ordinary messages */
      [notification_generator
        notificationOccurred:UINotificationFeedbackTypeError];
      [notification_generator prepare];
      break;
    }
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
