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

#ifndef XCSOAR_AUDIO_PCM_PLAYER_HPP
#define XCSOAR_AUDIO_PCM_PLAYER_HPP

#ifdef ANDROID
#define PCMPLAYER_SYNTHESISER_ONLY
#endif

#if !defined(ANDROID) && !defined(WIN32)

#include "Compiler.h"

#include <stddef.h>
#include <stdint.h>

#endif

#ifdef PCMPLAYER_SYNTHESISER_ONLY
class PCMSynthesiser;
#else
class PCMDataSource;
#endif

/**
 * Abstract base class for audio players that play 16 bit mono PCM data.  It is
 * being fed by a #PCMDataSource object that gets called when more
 * PCM samples are needed.
 */
class PCMPlayer {
public:
  /**
   * Start playback.
   *
   * @param source a PCMDataSource instance that will be used to
   * read sound; the caller is responsible for releasing it (not
   * before playback is stopped)
   */
#ifdef PCMPLAYER_SYNTHESISER_ONLY
  virtual bool Start(PCMSynthesiser &source) = 0;
#else
  virtual bool Start(PCMDataSource &source) = 0;
#endif

  /**
   * Stop playback and close the audio device.  This method is
   * synchronous.
   */
  virtual void Stop() = 0;

  virtual ~PCMPlayer() {}

protected:
#ifdef PCMPLAYER_SYNTHESISER_ONLY
  PCMSynthesiser *source = nullptr;
#else
  PCMDataSource *source = nullptr;
#endif

#if !defined(ANDROID) && !defined(WIN32)
  unsigned channels;

  /**
   * Fill a buffer with data from the player's data source.
   * Then perform upmixing if necessary.
   * If the buffer could not be filled with the requested amount of frames
   * completely, the rest of the frames is filled with silence (zeros).
   * @param buffer The buffer which is filled. Must be at least n * channels
   * elements long.
   * @param n The number of mono frames which should be read.
   * @return The number of read frames (an upmixed mono frame counts as one),
   * not including the silence frames.
   */
  gcc_nonnull_all
  size_t FillPCMBuffer(int16_t *buffer, size_t n);
#endif
};

#endif
