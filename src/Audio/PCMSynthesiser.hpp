/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

/**
 * This interface is used by PCMPlayer.
 */
class PCMSynthesiser {
public:
  /**
   * The caller requests to generate PCM samples.
   *
   * Note that this method may be called from any thread.  The
   * PCMSynthesiser implementation must be thread-safe.
   *
   * @param buffer the destination buffer (host byte order)
   * @param n the number of 16 bit mono samples that shall be generated
   */
  virtual void Synthesise(int16_t *buffer, size_t n) = 0;
};

#endif
