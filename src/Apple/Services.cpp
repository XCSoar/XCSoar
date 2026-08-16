// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef __APPLE__

#include <TargetConditionals.h>
#include "LogFile.hpp"
#include "Services.hpp"
#include "thread/Mutex.hxx"
#import <AVFoundation/AVFoundation.h>

#if TARGET_OS_IPHONE

/**
 * Serialises audio_vario_session_active against the deactivation of the
 * shared AVAudioSession. Without it, DeactivateAudioSession() could read
 * the flag as false, then have the audio vario start up (setting the
 * flag and activating the session) before its own setActive:NO takes
 * effect, silencing the freshly started vario.
 */
static Mutex audio_session_mutex;

/**
 * Protected by #audio_session_mutex: written from the SDL audio thread
 * (SDLPCMPlayer) and read from the thread calling SoundUtil::Play() and
 * from the AVAudioPlayer delegate callbacks.
 */
static bool audio_vario_session_active = false;

void
SetAudioVarioSessionActive(bool active)
{
  const std::lock_guard lock{audio_session_mutex};
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
  // hold the lock across the check and the deactivation, so that the
  // audio vario cannot start up in between and get silenced by our
  // setActive:NO
  const std::lock_guard lock{audio_session_mutex};

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
