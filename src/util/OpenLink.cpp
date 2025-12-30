// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OpenLink.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif

bool OpenLink(const char *url) noexcept {
#if defined(__APPLE__) && TARGET_OS_IPHONE
  NSString *ns_url = [NSString stringWithUTF8String:url];
  NSURL *nsu = [NSURL URLWithString:ns_url];
  if ([[UIApplication sharedApplication] canOpenURL:nsu]) {
    [[UIApplication sharedApplication] openURL:nsu options:@{} completionHandler:nil];
    return true;
  }
#endif
  (void)url;
  return false;
}
