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

/**
 * Deactivates the AVAudioSession once playback has finished, notifying
 * other apps so that any audio they had ducked or paused while our sound
 * was playing is restored. Without this, the session stays active
 * indefinitely and other apps' audio remains ducked forever.
 */
@interface XCSoarAudioPlayerDelegate : NSObject <AVAudioPlayerDelegate>
@end

static void
DeactivateAudioSession()
{
  NSError *error = nil;
  [[AVAudioSession sharedInstance] setActive:NO
                                   withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                   error:&error];
  if (error) {
    LogFormat("AVAudioSession deactivate error: %s", [[error localizedDescription] UTF8String]);
  }
}

@implementation XCSoarAudioPlayerDelegate

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)audioPlayer successfully:(BOOL)flag
{
  DeactivateAudioSession();
}

- (void)audioPlayerDecodeErrorDidOccur:(AVAudioPlayer *)audioPlayer error:(NSError *)error
{
  DeactivateAudioSession();
}

@end

static AVAudioPlayer *player = nil;
static XCSoarAudioPlayerDelegate *player_delegate = nil;
#endif

bool
SoundUtil::Play(const char *resource_name)
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

  // Activate the audio session for the duration of the playback. It is
  // deactivated again by the player delegate once playback has finished,
  // so that other apps' audio (e.g. music) is never ducked permanently.
  NSError *activate_error = nil;
  [[AVAudioSession sharedInstance] setActive:YES error:&activate_error];
  if (activate_error) {
    LogFormat("AVAudioSession activate error: %s", [[activate_error localizedDescription] UTF8String]);
  }

  if (!player_delegate) {
    player_delegate = [[XCSoarAudioPlayerDelegate alloc] init];
  }
  player.delegate = player_delegate;

  if (![player prepareToPlay] || ![player play]) {
    LogFormat("Failed to start playback for %s (%s)", resource_name, filename);
    DeactivateAudioSession();
    return false;
  }

  return true;
#else
  (void)resource_name;
  return false;
#endif
}
