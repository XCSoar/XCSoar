// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

// iOS implementation for sharing text using UIActivityViewController

#include "Apple/Share.hpp"
#include "util/ConvertString.hpp"

#if defined(__APPLE__) && TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#import <dispatch/dispatch.h>

static UIViewController *
GetTopMostController(UIViewController *controller) noexcept
{
  if (controller == nil)
    return nil;

  while (true) {
    if ([controller isKindOfClass:[UINavigationController class]]) {
      controller = ((UINavigationController *)controller).visibleViewController;
    } else if ([controller isKindOfClass:[UITabBarController class]]) {
      controller = ((UITabBarController *)controller).selectedViewController;
    } else if (controller.presentedViewController != nil) {
      controller = controller.presentedViewController;
    } else {
      break;
    }

    if (controller == nil)
      break;
  }

  return controller;
}

static UIViewController *
FindActiveController() noexcept
{
  UIWindow *activeWindow = nil;

  if (@available(iOS 13.0, *)) {
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
      if (scene.activationState != UISceneActivationStateForegroundActive)
        continue;

      if (![scene isKindOfClass:[UIWindowScene class]])
        continue;

      UIWindowScene *windowScene = (UIWindowScene *)scene;
      for (UIWindow *window in windowScene.windows) {
        if (window.isHidden)
          continue;

        if (window.isKeyWindow) {
          activeWindow = window;
          break;
        }

        if (activeWindow == nil)
          activeWindow = window;
      }

      if (activeWindow != nil)
        break;
    }
  }

  if (activeWindow == nil)
    activeWindow = UIApplication.sharedApplication.keyWindow;

  if (activeWindow == nil) {
    for (UIWindow *window in UIApplication.sharedApplication.windows) {
      if (window.isHidden)
        continue;

      activeWindow = window;
      if (window.isKeyWindow)
        break;
    }
  }

  if (activeWindow == nil)
    return nil;

  return GetTopMostController(activeWindow.rootViewController);
}

void
ShareTextIOS(const TCHAR *text) noexcept
{
  if (text == nullptr || *text == _T('\0'))
    return;

  const WideToUTF8Converter utf8{text};
  if (!utf8.IsValid())
    return;

  NSString *const shareString = [[NSString alloc] initWithUTF8String:utf8.c_str()];
  if (shareString == nil)
    return;

  dispatch_async(dispatch_get_main_queue(), ^{
    UIViewController *controller = FindActiveController();
    if (controller == nil)
      return;

    UIActivityViewController *activityVC =
      [[UIActivityViewController alloc] initWithActivityItems:@[shareString]
                                        applicationActivities:nil];

    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
      activityVC.modalPresentationStyle = UIModalPresentationPopover;
    }

    UIPopoverPresentationController *popover = activityVC.popoverPresentationController;
    if (popover != nil) {
      popover.sourceView = controller.view;
      popover.sourceRect = CGRectMake(CGRectGetMidX(controller.view.bounds),
                                      CGRectGetMidY(controller.view.bounds),
                                      0, 0);
      popover.permittedArrowDirections = 0;
    }

    [controller presentViewController:activityVC animated:YES completion:nil];
  });
}
#endif
