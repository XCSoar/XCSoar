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

#ifndef XCSOAR_AUDIO_ALGORITHMS_HPP
#define XCSOAR_AUDIO_ALGORITHMS_HPP

#include "Compiler.h"

#include <algorithm>

#include <assert.h>
#include <stddef.h>

/* Algorithms for processing audio data */

/**
 * Upmix mono PCM stream to a given number of channels
 */
template<typename T>
inline void UpmixMonoPCM(T *pcm_stream_buffer, size_t num_mono_frames,
                         size_t num_channels) {
  assert(nullptr != pcm_stream_buffer);
  assert(num_channels > 0);

  if (1 == num_channels)
      return;

  for (ssize_t i = static_cast<ssize_t>(num_mono_frames) - 1; i >= 0; --i) {
    std::fill(pcm_stream_buffer + i * num_channels,
              pcm_stream_buffer + i * num_channels + num_channels,
              pcm_stream_buffer[i]);
  }
}

#endif
