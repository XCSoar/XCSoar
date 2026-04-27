// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef __APPLE__

#include <TargetConditionals.h>
#include "LogFile.hpp"
#include "Services.hpp"
#import <AVFoundation/AVFoundation.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif

#if TARGET_OS_IPHONE
[[gnu::const]]
static const char *
ToString(UIApplicationState state) noexcept
{
  switch (state) {
  case UIApplicationStateActive:
    return "active";

  case UIApplicationStateInactive:
    return "inactive";

  case UIApplicationStateBackground:
    return "background";
  }

  return "unknown";
}

@interface XCSoarAppLifecycleObserver : NSObject
@end

@implementation XCSoarAppLifecycleObserver

- (void)registerNotifications
{
  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];

  [center addObserver:self
             selector:@selector(applicationDidBecomeActive:)
                 name:UIApplicationDidBecomeActiveNotification
               object:nil];
  [center addObserver:self
             selector:@selector(applicationWillResignActive:)
                 name:UIApplicationWillResignActiveNotification
               object:nil];
  [center addObserver:self
             selector:@selector(applicationDidEnterBackground:)
                 name:UIApplicationDidEnterBackgroundNotification
               object:nil];
  [center addObserver:self
             selector:@selector(applicationWillEnterForeground:)
                 name:UIApplicationWillEnterForegroundNotification
               object:nil];
  [center addObserver:self
             selector:@selector(applicationWillTerminate:)
                 name:UIApplicationWillTerminateNotification
               object:nil];
  [center addObserver:self
             selector:@selector(applicationDidReceiveMemoryWarning:)
                 name:UIApplicationDidReceiveMemoryWarningNotification
               object:nil];
}

- (void)unregisterNotifications
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
  (void)notification;
  LogFormat("iOS lifecycle: did become active (state=%s)",
            ToString(UIApplication.sharedApplication.applicationState));
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
  (void)notification;
  LogFormat("iOS lifecycle: will resign active (state=%s)",
            ToString(UIApplication.sharedApplication.applicationState));
}

- (void)applicationDidEnterBackground:(NSNotification *)notification
{
  (void)notification;
  LogFormat("iOS lifecycle: did enter background (state=%s)",
            ToString(UIApplication.sharedApplication.applicationState));
}

- (void)applicationWillEnterForeground:(NSNotification *)notification
{
  (void)notification;
  LogFormat("iOS lifecycle: will enter foreground (state=%s)",
            ToString(UIApplication.sharedApplication.applicationState));
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
  (void)notification;
  LogFormat("iOS lifecycle: will terminate (state=%s)",
            ToString(UIApplication.sharedApplication.applicationState));
}

- (void)applicationDidReceiveMemoryWarning:(NSNotification *)notification
{
  (void)notification;
  LogFormat("iOS lifecycle: did receive memory warning (state=%s)",
            ToString(UIApplication.sharedApplication.applicationState));
}

@end

static XCSoarAppLifecycleObserver *app_lifecycle_observer = nil;
#endif

// Initialize apple services - this will be called from the main XCSoar startup
void
InitializeAppleServices()
{
#if TARGET_OS_IPHONE
  if (app_lifecycle_observer == nil) {
    app_lifecycle_observer = [[XCSoarAppLifecycleObserver alloc] init];
    [app_lifecycle_observer registerNotifications];
    LogFormat("iOS lifecycle: observer installed (state=%s)",
              ToString(UIApplication.sharedApplication.applicationState));
  }

  // Setup AVAudioSession for better audio playback
  NSError *error = nil;
  AVAudioSession *session = [AVAudioSession sharedInstance];
  [session setCategory:AVAudioSessionCategoryPlayback
           withOptions:AVAudioSessionCategoryOptionMixWithOthers
                 error:&error];
  [session setActive:YES error:&error];
  if (error) {
    LogFormat("AVAudioSession initialize error: %s", [[error localizedDescription] UTF8String]);
  }
#endif
}

// Cleanup apple services - this will be called from XCSoar shutdown
void
DeinitializeAppleServices()
{
#if TARGET_OS_IPHONE
  if (app_lifecycle_observer != nil) {
    [app_lifecycle_observer unregisterNotifications];
    app_lifecycle_observer = nil;
    LogString("iOS lifecycle: observer removed");
  }

  // Deinitialize AVAudioSession
  NSError *error = nil;
  [[AVAudioSession sharedInstance] setActive:NO error:&error];
  if (error) {
    LogFormat("AVAudioSession deinitialize error: %s", [[error localizedDescription] UTF8String]);
  }
#endif
}

#endif
