// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DarkMode.hpp"
#include "GlobalSettings.hpp"

#include <TargetConditionals.h>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

static bool
IsSystemDarkMode() noexcept
{
#if TARGET_OS_IPHONE
  if (@available(iOS 13.0, *)) {
    /* the window scene's trait collection is the authoritative source;
       UITraitCollection.currentTraitCollection is only guaranteed to
       be up to date inside UIKit callbacks, which this is not */
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
      if ([scene isKindOfClass:UIWindowScene.class])
        return ((UIWindowScene *)scene).traitCollection.userInterfaceStyle ==
          UIUserInterfaceStyleDark;

    return UITraitCollection.currentTraitCollection.userInterfaceStyle ==
      UIUserInterfaceStyleDark;
  }

  /* iOS 12 and older have no dark appearance */
  return false;
#else
  NSApplication *application = NSApp;
  if (application == nil)
    return false;

  NSAppearanceName name =
    [application.effectiveAppearance bestMatchFromAppearancesWithNames:
     @[NSAppearanceNameAqua, NSAppearanceNameDarkAqua]];
  return [name isEqualToString:NSAppearanceNameDarkAqua];
#endif
}

bool
UpdateAppleDarkMode() noexcept
{
  const bool dark_mode = IsSystemDarkMode();
  if (dark_mode == GlobalSettings::dark_mode)
    return false;

  GlobalSettings::dark_mode = dark_mode;
  return true;
}
