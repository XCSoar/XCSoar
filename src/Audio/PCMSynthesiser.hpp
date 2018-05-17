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

#ifndef XCSOAR_AUDIO_PCM_SYNTHESISER_HPP
#define XCSOAR_AUDIO_PCM_SYNTHESISER_HPP

#include <stddef.h>
#include <stdint.h>

#include "PCMDataSource.hpp"
#include "OS/ByteOrder.hpp"

class PCMSynthesiser : public PCMDataSource {
public:
  /**
   * The caller requests to generate PCM samples.
   *
   * @param buffer the destination buffer (host byte order)
   * @param n the number of 16 bit mono samples that shall be generated
   */
  virtual void Synthesise(int16_t *buffer, size_t n) = 0;

  /* virtual methods from class PCMDataSource */

  bool IsBigEndian() const {
    /* Our PCM synthesisers always deliver data in host byte order */
    return ::IsBigEndian();
  }

  size_t GetData(int16_t *buffer, size_t n) {
    Synthesise(buffer, n);
    return n;
  }
};

#endif
