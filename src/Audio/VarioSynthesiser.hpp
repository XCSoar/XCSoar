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

#ifndef XCSOAR_AUDIO_VARIO_SYNTHESISER_HPP
#define XCSOAR_AUDIO_VARIO_SYNTHESISER_HPP

#include "ToneSynthesiser.hpp"
#include "Math/fixed.hpp"
#include "Compiler.h"

/**
 * This class generates vario sound.
 */
class VarioSynthesiser : public ToneSynthesiser {
  /**
   * The number of audible samples in each period.
   */
  size_t audible_count;

  /**
   * The number of silent samples in each period.  If this is zero,
   * then no silence will be generated (continuous tone).
   */
  size_t silence_count;

  /**
   * The number of audible/silence samples remaining in the current
   * period.  These two attributes will be reset to the according
   * _count value when both reach zero.
   */
  size_t audible_remaining, silence_remaining;

  bool dead_band_enabled;

public:
  VarioSynthesiser()
    :audible_count(0), silence_count(1),
     audible_remaining(0), silence_remaining(0),
     dead_band_enabled(true) {}

  /**
   * Update the vario value.  This calculates a new tone frequency and
   * a new "silence" rate (for positive vario values).
   *
   * @param vario the current vario value [m/s]
   */
  void SetVario(unsigned sample_rate, fixed vario);

  /**
   * Produce silence from now on.
   */
  void SetSilence();

  void SetDeadBand(bool enabled) {
    dead_band_enabled = enabled;
  }

  /* methods from class PCMSynthesiser */
  virtual void Synthesise(int16_t *buffer, size_t n);
};

#endif
