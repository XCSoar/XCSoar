/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include <stddef.h>

class PCMSynthesiser;

/**
 * An audio player that plays synthesized 16 bit mono PCM data.  It is
 * being fed by a #PCMSynthesiser object that gets called when more
 * PCM samples are needed.
 */
class PCMPlayer {
  unsigned sample_rate;

  PCMSynthesiser *synthesiser;

#ifdef ANDROID
#elif defined(WIN32)
#else
#endif

public:
  PCMPlayer();
  ~PCMPlayer();

  /**
   * Start playback.
   *
   * @param synthesiser a PCMSynthesiser instance that will be used to
   * generate sound; the caller is responsible for releasing it (not
   * before playback is stopped)
   */
  bool Start(PCMSynthesiser &synthesiser, unsigned sample_rate);

  /**
   * Stop playback and close the audio device.  This method is
   * synchronous.
   */
  void Stop();

#ifdef ANDROID
#elif defined(WIN32)
#else
  void Synthesise(void *buffer, size_t n);
#endif
};

#endif
