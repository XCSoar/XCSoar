// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef __APPLE__

#include <TargetConditionals.h>
#include "LogFile.hpp"
#include "Services.hpp"
#import <AVFoundation/AVFoundation.h>

#if TARGET_OS_IPHONE

static bool audio_vario_session_active = false;

void
SetAudioVarioSessionActive(bool active)
{
  audio_vario_session_active = active;
}

void
ActivateAudioSession()
{
  NSError *error = nil;
  AVAudioSession *session = [AVAudioSession sharedInstance];

  // (Re-)apply our preferred category and options. SDL's CoreAudio
  // backend may reset these when it (re-)opens the audio device for the
  // audio vario, which would otherwise cause XCSoar to duck other apps'
  // audio.
  [session setCategory:AVAudioSessionCategoryPlayback
           withOptions:AVAudioSessionCategoryOptionMixWithOthers
                 error:&error];
  if (error) {
    LogFormat("AVAudioSession setCategory error: %s", [[error localizedDescription] UTF8String]);
    error = nil;
  }

  [session setActive:YES error:&error];
  if (error) {
    LogFormat("AVAudioSession activate error: %s", [[error localizedDescription] UTF8String]);
  }
}

void
DeactivateAudioSession()
{
  if (audio_vario_session_active) {
    // keep the session active while the audio vario's audio device is
    // open: deactivating it would also silence the audio vario, which
    // SDL would not resume on its own
    return;
  }

  NSError *error = nil;
  [[AVAudioSession sharedInstance] setActive:NO
                                   withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                   error:&error];
  if (error) {
    LogFormat("AVAudioSession deactivate error: %s", [[error localizedDescription] UTF8String]);
  }
}

#endif

// Initialize apple services - this will be called from the main XCSoar startup
void
InitializeAppleServices()
{
#if TARGET_OS_IPHONE
  ActivateAudioSession();
#endif
}

// Cleanup apple services - this will be called from XCSoar shutdown
void
DeinitializeAppleServices()
{
#if TARGET_OS_IPHONE
  // Deinitialize AVAudioSession
  NSError *error = nil;
  [[AVAudioSession sharedInstance] setActive:NO
                                   withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                   error:&error];
  if (error) {
    LogFormat("AVAudioSession deinitialize error: %s", [[error localizedDescription] UTF8String]);
  }
#endif
}

#endif
