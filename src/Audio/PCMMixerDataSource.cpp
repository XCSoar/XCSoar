// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PCMMixerDataSource.hpp"

#include "AudioAlgorithms.hpp"

#include "util/Macros.hpp"

#include <cassert>

#include <algorithm>

bool
PCMMixerDataSource::AddSource(PCMDataSource &source)
{
  assert(source.GetSampleRate() == sample_rate);

  const std::lock_guard protect{lock};

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
  const std::lock_guard protect{lock};

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

  const std::lock_guard protect{lock};
  vol_percent = _vol_percent;
}

size_t
PCMMixerDataSource::GetData(int16_t *buffer, size_t n)
{
  size_t copied_count = 0;

  PCMDataSource *sources_to_remove[MAX_MIXER_SOURCES_COUNT];
  unsigned sources_to_remove_count = 0;

  const std::lock_guard protect{lock};

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
