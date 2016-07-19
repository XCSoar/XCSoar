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

#include "PCMMixerDataSource.hpp"

#include "AudioAlgorithms.hpp"

#include "Util/Macros.hpp"

#include <assert.h>

#include <algorithm>

bool
PCMMixerDataSource::AddSource(PCMDataSource &source)
{
  assert(source.GetSampleRate() == sample_rate);

  const ScopeLock protect(lock);

#ifndef NDEBUG
  for (unsigned i = 0; i < MAX_MIXER_SOURCES_COUNT; ++i) {
    assert(sources[i] != &source);
  }
#endif

  for (unsigned i = 0; i < MAX_MIXER_SOURCES_COUNT; ++i) {
    if (nullptr == sources[i]) {
      sources[i] = &source;
      return true;
    }
  }

  return false;
}

void
PCMMixerDataSource::RemoveSource(PCMDataSource &source)
{
  const ScopeLock protect(lock);

  for (unsigned i = 0; i < MAX_MIXER_SOURCES_COUNT; ++i) {
    if (sources[i] == &source) {
      sources[i] = nullptr;
      return;
    }
  }
}

void
PCMMixerDataSource::SetVolume(unsigned _vol_percent)
{
  assert(_vol_percent <= 100);

  const ScopeLock protect(lock);
  vol_percent = _vol_percent;
}

size_t
PCMMixerDataSource::GetData(int16_t *buffer, size_t n)
{
  size_t copied_count = 0;

  PCMDataSource *sources_to_remove[MAX_MIXER_SOURCES_COUNT];
  unsigned sources_to_remove_count = 0;

  const ScopeLock protect(lock);

  for (unsigned i = 0; i < MAX_MIXER_SOURCES_COUNT; ++i) {
    PCMDataSource *source = sources[i];
    if (nullptr != source) {
      bool do_byte_swap = ::IsBigEndian() != source->IsBigEndian();
      if (0 == copied_count) {
        copied_count = source->GetData(buffer, n);
        if (0 == copied_count) {
          sources_to_remove[sources_to_remove_count++] = source;
        } else {
          if (do_byte_swap)
            ByteSwapAndLowerVolume(buffer, copied_count, vol_percent);
          else
            LowerVolume(buffer, copied_count, vol_percent);
        }
      } else {
        int16_t temp_buffer[1024];
        size_t current_src_copied_count = 0;
        bool current_src_end = false;
        while (!current_src_end && (current_src_copied_count < copied_count)) {
          size_t bytes_to_read = std::min(copied_count - current_src_copied_count,
                                          ARRAY_SIZE(temp_buffer));
          size_t iteration_read_count =
              source->GetData(temp_buffer, bytes_to_read);
          if (do_byte_swap)
            ByteSwapAndMixPCM(buffer + current_src_copied_count, temp_buffer,
                              iteration_read_count, vol_percent);
          else
            MixPCM(buffer + current_src_copied_count, temp_buffer,
                   iteration_read_count, vol_percent);
          current_src_copied_count += iteration_read_count;
          current_src_end = (iteration_read_count < bytes_to_read);
        }
        if (!current_src_end && (copied_count < n)) {
          size_t bytes_to_read = n - copied_count;
          size_t rest_read_count =
              source->GetData(buffer + copied_count, bytes_to_read);
          if (do_byte_swap)
            ByteSwapAndLowerVolume(buffer + copied_count, rest_read_count,
                                   vol_percent);
          else
            LowerVolume(buffer + copied_count, rest_read_count,
                        vol_percent);
          copied_count += rest_read_count;
          current_src_end = (rest_read_count < bytes_to_read);
        }
        if (current_src_end) {
          sources_to_remove[sources_to_remove_count++] = source;
        }
      }
    }
  }

  if (sources_to_remove_count > 0) {
    for (unsigned i = 0; i < sources_to_remove_count; ++i) {
      for (unsigned j = 0; j < MAX_MIXER_SOURCES_COUNT; ++j) {
        if (sources_to_remove[i] == sources[j])
          sources[j] = nullptr;
      }
    }
  }

  return copied_count;
}
