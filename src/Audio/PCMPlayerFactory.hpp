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

#ifndef XCSOAR_AUDIO_PCM_PLAYER_FACTORY_HPP
#define XCSOAR_AUDIO_PCM_PLAYER_FACTORY_HPP

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

namespace boost { namespace asio { class io_service; }}

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
CreateInstanceForDirectAccess(boost::asio::io_service &io_service)
{
#if defined(ENABLE_SDL)
  return new SDLPCMPlayer();
#elif defined(ENABLE_ALSA)
  return new ALSAPCMPlayer(io_service);
#else
  return CreateInstance();
#endif
}

}

#endif
