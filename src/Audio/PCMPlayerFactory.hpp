// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ANDROID
#include "AndroidPCMPlayer.hpp"
#elif defined(ENABLE_ALSA)
#include "ALSAPCMPlayer.hpp"
#elif defined(ENABLE_SDL)
#include "SDLPCMPlayer.hpp"
#endif

#if defined(ENABLE_SDL) || defined(ENABLE_ALSA)
#include "MixerPCMPlayer.hpp"
#endif

class EventLoop;

namespace PCMPlayerFactory
{

/**
 * Create an instance of a PCMPlayer implementation for the current platform
 * @return Pointer to the created PCMPlayer instance
 */
inline PCMPlayer *CreateInstance()
{
#ifdef ANDROID
  return new AndroidPCMPlayer();
#elif defined(ENABLE_SDL) || defined(ENABLE_ALSA)
  return new MixerPCMPlayer();
#else
#error No PCMPlayer implementation available
#endif
}

/**
 * Create an instance of a PCMPlayer implementation for the current platform,
 * for direct access to the audio device (which can maybe not be used by
 * multiple clients at the same time).
 * @return Pointer to the created PCMPlayer instance
 */
inline PCMPlayer *
CreateInstanceForDirectAccess([[maybe_unused]] EventLoop &event_loop)
{
#if defined(ENABLE_SDL)
  return new SDLPCMPlayer();
#elif defined(ENABLE_ALSA)
  return new ALSAPCMPlayer(event_loop);
#else
  return CreateInstance();
#endif
}

}
