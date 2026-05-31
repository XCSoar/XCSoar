// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RunFile.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
#import <UIKit/UIKit.h>

@interface XCSoarDocumentInteractionDelegate
  : NSObject <UIDocumentInteractionControllerDelegate>
@end

static XCSoarDocumentInteractionDelegate *document_interaction_delegate;
static UIDocumentInteractionController *document_interaction_controller;

static UIWindow *
GetApplicationWindow() noexcept
{
  if (@available(iOS 13.0, *)) {
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
      if (scene.activationState != UISceneActivationStateForegroundActive ||
          ![scene isKindOfClass:UIWindowScene.class])
        continue;

      for (UIWindow *window in ((UIWindowScene *)scene).windows)
        if (window.isKeyWindow)
          return window;
    }
  }

  return nil;
}

static UIViewController *
GetTopViewController() noexcept
{
  UIWindow *window = GetApplicationWindow();
  UIViewController *controller = window.rootViewController;
  while (controller.presentedViewController != nil)
    controller = controller.presentedViewController;

  return controller;
}

@implementation XCSoarDocumentInteractionDelegate

- (UIViewController *)documentInteractionControllerViewControllerForPreview:
  (UIDocumentInteractionController *)controller
{
  (void)controller;
  return GetTopViewController();
}

- (void)documentInteractionControllerDidEndPreview:
  (UIDocumentInteractionController *)controller
{
  (void)controller;
  document_interaction_controller = nil;
}

@end

static bool
RunFileIOS(NSURL *url) noexcept
{
  UIViewController *controller = GetTopViewController();
  if (controller == nil)
    return false;

  if (document_interaction_delegate == nil)
    document_interaction_delegate =
      [[XCSoarDocumentInteractionDelegate alloc] init];

  document_interaction_controller =
    [UIDocumentInteractionController interactionControllerWithURL:url];
  document_interaction_controller.delegate = document_interaction_delegate;

  if (![document_interaction_controller presentPreviewAnimated:YES]) {
    document_interaction_controller = nil;
    return false;
  }

  return true;
}
#endif

#ifdef ANDROID
// replaced by NativeView::OpenWaypointFile()
#elif defined(HAVE_POSIX) && !defined(_WIN32) && !defined(KOBO)

#include "Process.hpp"

bool
RunFile(const char *path) noexcept
{
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

#if defined(__APPLE__) && TARGET_OS_IPHONE
  NSString *ns_path = [NSString stringWithUTF8String:path];
  if (ns_path == nil)
    return false;

  NSURL *url = [NSURL fileURLWithPath:ns_path];
  if (url == nil)
    return false;

  if (NSThread.isMainThread)
    return RunFileIOS(url);

  __block bool result = false;
  dispatch_sync(dispatch_get_main_queue(), ^{
    result = RunFileIOS(url);
  });
  return result;
#elif defined(__APPLE__)
  return Start("/usr/bin/open", path);
#else
  return Start("/usr/bin/xdg-open", path);
#endif
}

#endif
