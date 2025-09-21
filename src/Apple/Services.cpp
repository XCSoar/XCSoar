// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Services.hpp"
#include "LogFile.hpp"
#import <AVFoundation/AVFoundation.h>

// Initialize apple services - this will be called from the main XCSoar startup
void
InitializeAppleServices()
{
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
}

// Cleanup apple services - this will be called from XCSoar shutdown
void
DeinitializeAppleServices()
{
  // Deinitialize AVAudioSession
  NSError *error = nil;
  [[AVAudioSession sharedInstance] setActive:NO error:&error];
  if (error) {
    LogFormat("AVAudioSession deinitialize error: %s", [[error localizedDescription] UTF8String]);
  }
}
