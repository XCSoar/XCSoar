// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

// Implementation of SoundUtil using the AVFoundation framework 
// Currently for iOS only, not macOS (AVAudioSession is iOS only)

#include "SoundUtil.hpp"
#include "Services.hpp"
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
 *
 * (DeactivateAudioSession() skips the deactivation while the audio
 * vario is active, so it keeps playing.)
 */
@interface XCSoarAudioPlayerDelegate : NSObject <AVAudioPlayerDelegate>
@end

static bool PlayNextQueued();

@implementation XCSoarAudioPlayerDelegate

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)audioPlayer successfully:(BOOL)flag
{
  if (!PlayNextQueued())
    DeactivateAudioSession();
}

- (void)audioPlayerDecodeErrorDidOccur:(AVAudioPlayer *)audioPlayer error:(NSError *)error
{
  DeactivateAudioSession();
}

@end

static AVAudioPlayer *player = nil;
static XCSoarAudioPlayerDelegate *player_delegate = nil;

/** The clips of a sequence still to play after the current one */
static NSMutableArray<NSString *> *queued_files = nil;

/**
 * Map a resource name to the name of its WAV file in the app bundle.
 * ToDo: Avoid duplication of static mapping information with
 * android/src/SoundUtil.java ?
 */
static const char *
GetFileName(const char *resource_name)
{
  static constexpr struct {
    const char *resource_name, *filename;
  } files[] = {
    { "IDR_FAIL", "fail" },
    { "IDR_INSERT", "insert" },
    { "IDR_REMOVE", "remove" },
    { "IDR_WAV_BEEPBWEEP", "beep_bweep" },
    { "IDR_WAV_CLEAR", "beep_clear" },
    { "IDR_WAV_DRIP", "beep_drip" },
    { "IDR_WAV_TRAFFIC_LOW", "traffic_low" },
    { "IDR_WAV_TRAFFIC_IMPORTANT", "traffic_important" },
    { "IDR_WAV_TRAFFIC_URGENT", "traffic_urgent" },
    { "IDR_WAV_OCLOCK_1", "oclock_1" },
    { "IDR_WAV_OCLOCK_2", "oclock_2" },
    { "IDR_WAV_OCLOCK_3", "oclock_3" },
    { "IDR_WAV_OCLOCK_4", "oclock_4" },
    { "IDR_WAV_OCLOCK_5", "oclock_5" },
    { "IDR_WAV_OCLOCK_6", "oclock_6" },
    { "IDR_WAV_OCLOCK_7", "oclock_7" },
    { "IDR_WAV_OCLOCK_8", "oclock_8" },
    { "IDR_WAV_OCLOCK_9", "oclock_9" },
    { "IDR_WAV_OCLOCK_10", "oclock_10" },
    { "IDR_WAV_OCLOCK_11", "oclock_11" },
    { "IDR_WAV_OCLOCK_12", "oclock_12" },
    { "IDR_WAV_ABOVE", "above" },
    { "IDR_WAV_BELOW", "below" },
    { "IDR_WAV_LEVEL", "level" },
    { "IDR_WAV_AIRSPACE_NEAR", "airspace_near" },
    { "IDR_WAV_AIRSPACE_ABOVE", "airspace_above" },
    { "IDR_WAV_AIRSPACE_INSIDE", "airspace_inside" },
  };

  for (const auto &i : files)
    if (strcmp(resource_name, i.resource_name) == 0)
      return i.filename;

  LogFmt("Unknown sound resource: {}", resource_name);
  return nullptr;
}

/**
 * Start playing a WAV file from the app bundle, replacing the sound
 * which is playing.
 */
static bool
StartFile(NSString *filename)
{
  // Construct path to WAV file in the app bundle
  NSString *mainBundlePath = [[NSBundle mainBundle] bundlePath];
  NSString *wavPath = [mainBundlePath stringByAppendingPathComponent:
                       [NSString stringWithFormat:@"%@.wav", filename]];

  NSError *error = nil;
  NSURL *fileURL = [NSURL fileURLWithPath:wavPath];

  player = [[AVAudioPlayer alloc] initWithContentsOfURL:fileURL error:&error];

  if (!player) {
    if (error) {
      LogFmt("Failed to create AVAudioPlayer for {}: {}",
             [filename UTF8String],
             [[error localizedDescription] UTF8String]);
    } else {
      LogFmt("Failed to create AVAudioPlayer for {}: unknown reason",
             [filename UTF8String]);
    }
    return false;
  }

  // Activate the audio session for the duration of the playback. It is
  // deactivated again by the player delegate once playback has finished,
  // so that other apps' audio (e.g. music) is never ducked permanently.
  ActivateAudioSession();

  if (!player_delegate) {
    player_delegate = [[XCSoarAudioPlayerDelegate alloc] init];
  }
  player.delegate = player_delegate;

  if (![player prepareToPlay] || ![player play]) {
    LogFmt("Failed to start playback for {}", [filename UTF8String]);
    DeactivateAudioSession();
    return false;
  }

  return true;
}

/**
 * Start the next clip of a sequence.
 *
 * @return false if there is none left
 */
static bool
PlayNextQueued()
{
  while (queued_files.count > 0) {
    NSString *filename = queued_files.firstObject;
    [queued_files removeObjectAtIndex:0];
    if (StartFile(filename))
      return true;
  }

  return false;
}
#endif

bool
SoundUtil::Play(const char *resource_name)
{
#if TARGET_OS_IPHONE
  const char *filename = GetFileName(resource_name);
  if (filename == nullptr)
    return false;

  [queued_files removeAllObjects];
  return StartFile([NSString stringWithUTF8String:filename]);
#else
  (void)resource_name;
  return false;
#endif
}

bool
SoundUtil::PlaySequence(std::span<const char *const> resource_names)
{
#if TARGET_OS_IPHONE
  if (!queued_files)
    queued_files = [[NSMutableArray alloc] init];

  [queued_files removeAllObjects];
  for (const char *resource_name : resource_names) {
    const char *filename = GetFileName(resource_name);
    if (filename != nullptr)
      [queued_files addObject:[NSString stringWithUTF8String:filename]];
  }

  return PlayNextQueued();
#else
  (void)resource_names;
  return false;
#endif
}
