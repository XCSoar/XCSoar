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

#ifndef XCSOAR_AUDIO_PCM_DATA_SOURCE_HPP
#define XCSOAR_AUDIO_PCM_DATA_SOURCE_HPP

#include <stddef.h>
#include <stdint.h>

/**
 * This interface is used by PCMPlayer.
 */
class PCMDataSource {
public:
  /**
   * Check if this data source delivers Big Endian PCM samples.
   *
   * @return true means it delivers Big Endian samples, false means it
   * delivers Little Endian samples
   */
  virtual bool IsBigEndian() const = 0;

  /**
   * Get the sample rate in hz which this PCM data source delivers.
   */
  virtual unsigned GetSampleRate() const = 0;

  /**
   * The caller requests PCM data.
   *
   * @param buffer the destination buffer
   * @param n the number of requested 16 bit mono samples
   *
   * @return the number of delivered samples. If less than n, it means that
   * playback for this data source is to be stopped after playing the returned
   * number of samples.
   */
  virtual size_t GetData(int16_t *buffer, size_t n) = 0;
};

#endif
