/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

/** \file
 *
 * This header provides macros and inline functions providing
 * information about the availability of audio playback features.
 */

#ifndef XCSOAR_AUDIO_FEATURES_HPP
#define XCSOAR_AUDIO_FEATURES_HPP

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

#endif
