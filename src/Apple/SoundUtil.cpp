// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

// Implementation of SoundUtil using the AVFoundation framework 
// Currently for iOS only, not macOS (AVAudioSession is iOS only)

#include "SoundUtil.hpp"
#include "LogFile.hpp"

#include <TargetConditionals.h>

#if TARGET_OS_IPHONE
#import <AVFoundation/AVFoundation.h>
#endif
#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE
static AVAudioPlayer *player = nil;
#endif

bool
SoundUtil::Play(const TCHAR *resource_name)
{
#if TARGET_OS_IPHONE
  // Map resource names to actual file names
  // ToDo: Avoid duplication of static mapping information with android/src/SoundUtil.java ?
  const char *filename = nullptr;
  if (strcmp(resource_name, "IDR_FAIL") == 0) {
    filename = "fail";
  } else if (strcmp(resource_name, "IDR_INSERT") == 0) {
    filename = "insert";
  } else if (strcmp(resource_name, "IDR_REMOVE") == 0) {
    filename = "remove";
  } else if (strcmp(resource_name, "IDR_WAV_BEEPBWEEP") == 0) {
    filename = "beep_bweep";
  } else if (strcmp(resource_name, "IDR_WAV_CLEAR") == 0) {
    filename = "beep_clear";
  } else if (strcmp(resource_name, "IDR_WAV_DRIP") == 0) {
    filename = "beep_drip";
  } else {
    LogFormat("Unknown sound resource: %s", resource_name);
    return false;
  }
  
  // Construct path to WAV file in the app bundle
  NSString *filenameStr = [NSString stringWithUTF8String:filename];
  NSString *mainBundlePath = [[NSBundle mainBundle] bundlePath];
  NSString *wavPath = [mainBundlePath stringByAppendingPathComponent:
                       [NSString stringWithFormat:@"%@.wav", filenameStr]];
  
  NSError *error = nil;
  NSURL *fileURL = [NSURL fileURLWithPath:wavPath];
  
  player = [[AVAudioPlayer alloc] initWithContentsOfURL:fileURL error:&error];

  if (!player) {
    if (error) {
      LogFormat("Failed to create AVAudioPlayer for %s (%s): %s", resource_name, filename,
                [[error localizedDescription] UTF8String]);
    } else {
      LogFormat("Failed to create AVAudioPlayer for %s (%s): unknown reason", resource_name, filename);
    }
    return false;
  }

  [player prepareToPlay];
  [player play];

  return true;
#else
  (void)resource_name;
  return false;
#endif
}
