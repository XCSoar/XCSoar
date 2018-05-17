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

#ifndef XCSOAR_AUDIO_PCM_BUFFER_DATA_SOURCE_HPP
#define XCSOAR_AUDIO_PCM_BUFFER_DATA_SOURCE_HPP

#include "PCMDataSource.hpp"

#include "Thread/Mutex.hpp"
#include "Util/ConstBuffer.hxx"

#include <stdint.h>

#include <list>

/**
 * A #PCMDataSource for playback of memory buffers with raw PCM data.
 *
 * The PCM data must be mono, 16 bit (signed), big endian with 44100 Hz sampe
 * rate.
 */
class PCMBufferDataSource : public PCMDataSource {
public:
  typedef ConstBuffer<int16_t> PCMData;

private:
  Mutex lock;

  std::list<PCMData> queued_data;
  unsigned offset;

public:
  /**
   * Add a #PCMData buffer to the queue.
   *
   * @return Count of queued #PCMData buffers (including the added one).
   */
  unsigned Add(PCMData &&data);

  /**
   * Clear the queue.
   */
  void Clear();

  /* virtual methods from class PCMDataSource */
  bool IsBigEndian() const override {
    return false; // Our PCM resources are always little endian
  }

  unsigned GetSampleRate() const override {
    return 44100; // Our PCM resources have this sample rate
  }

  size_t GetData(int16_t *buffer, size_t n) override;
};

#endif
