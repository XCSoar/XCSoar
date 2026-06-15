// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

void InitializeAppleServices();
void DeinitializeAppleServices();

#ifdef __APPLE__

#include <TargetConditionals.h>

#if TARGET_OS_IPHONE

/**
 * (Re-)applies XCSoar's preferred AVAudioSession category and options
 * (Playback, MixWithOthers) and activates the session, so that XCSoar's
 * own audio never ducks or interrupts other apps' audio.
 *
 * This must be called again whenever something else (e.g. SDL's
 * CoreAudio backend, when it (re-)opens the audio device for the audio
 * vario) may have reset the session's category or options.
 */
void ActivateAudioSession();

/**
 * Deactivates the shared AVAudioSession, notifying other apps so that
 * any audio they had ducked while ours was playing is restored.
 *
 * Does nothing while the audio vario's audio device is marked active
 * (see SetAudioVarioSessionActive()), because deactivating the session
 * would also silence the audio vario, which SDL would not resume on its
 * own.
 */
void DeactivateAudioSession();

/**
 * Tells the shared AVAudioSession helpers whether the audio vario's
 * audio device is currently open, so DeactivateAudioSession() knows
 * whether it is safe to deactivate the session after a one-shot sound
 * has finished playing.
 */
void SetAudioVarioSessionActive(bool active);

#endif
#endif
