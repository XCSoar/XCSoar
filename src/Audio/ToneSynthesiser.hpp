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

#ifndef XCSOAR_AUDIO_TONE_SYNTHESISER_HPP
#define XCSOAR_AUDIO_TONE_SYNTHESISER_HPP

#include "PCMSynthesiser.hpp"
#include "Compiler.h"

/**
 * This class generates tones with a sine wave.
 */
class ToneSynthesiser : public PCMSynthesiser {
  unsigned volume = 100, angle = 0, increment = 0;

public:
  explicit ToneSynthesiser(unsigned _sample_rate) : sample_rate(_sample_rate) {
  }

  unsigned GetSampleRate() const {
    return sample_rate;
  }

  /**
   * Set the (software) volume of the generated tone.
   *
   * @param _volume the new volume level, 0 indicating muted, 100
   * means full volume
   */
  void SetVolume(unsigned _volume) {
    volume = _volume;
  }

  void SetTone(unsigned tone_hz);

  /* methods from class PCMSynthesiser */
  virtual void Synthesise(int16_t *buffer, size_t n);

protected:
  const unsigned sample_rate;

  /**
   * Returns the number of samples until the sample value gets close
   * to zero.
   */
  gcc_pure
  unsigned ToZero() const;

  /**
   * Start a new period.
   */
  void Restart() {
    angle = 0;
  }
};

#endif
