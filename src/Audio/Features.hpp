// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/** \file
 *
 * This header provides macros and inline functions providing
 * information about the availability of audio playback features.
 */

#pragma once

#if defined(ENABLE_SDL) || defined(ANDROID) || defined(ENABLE_ALSA)
#define HAVE_PCM_PLAYER
#endif

#if defined(HAVE_PCM_PLAYER) && (defined(ENABLE_SDL) || defined(ENABLE_ALSA))
#define HAVE_PCM_MIXER
#endif

/* When the  PCM mixer is being used, we can at least control volume through
 * #PCMMixer. */
#ifdef HAVE_PCM_MIXER
#define HAVE_VOLUME_CONTROLLER
#endif

/* When HAVE_EXT_VOLUME_CONTROLLER is defined, it means that XCSoar takes
 * control of the audio device's volume level.
 * For now, this is only enabled for Kobo, where a volume controller
 * application is not easily available.
 * It might make sense to enable it for other embeddded targets as well. */
#if defined(KOBO) && defined(ENABLE_ALSA)
#define HAVE_EXT_VOLUME_CONTROLLER
#endif

constexpr
static inline bool
HavePCMPlayer()
{
#ifdef HAVE_PCM_PLAYER
  return true;
#else
  return false;
#endif
}

constexpr
static inline bool
HavePCMMixer()
{
#ifdef HAVE_PCM_MIXER
  return true;
#else
  return false;
#endif
}

constexpr
static inline bool
HaveVolumeController()
{
#ifdef HAVE_VOLUME_CONTROLLER
  return true;
#else
  return false;
#endif
}

constexpr
static inline bool
HaveExtVolumeController()
{
#ifdef HAVE_EXT_VOLUME_CONTROLLER
  return true;
#else
  return false;
#endif
}
