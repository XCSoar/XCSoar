// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PCMBufferDataSource.hpp"

#include <algorithm>

unsigned
PCMBufferDataSource::Add(PCMData &&data)
{
  const std::lock_guard protect{lock};

  queued_data.emplace_back(data);
  unsigned size = queued_data.size();
  if (1 == size)
    offset = 0;

  return size;
}

void
PCMBufferDataSource::Clear()
{
  const std::lock_guard protect{lock};
  queued_data.clear();
}

size_t
PCMBufferDataSource::GetData(int16_t *buffer, size_t n)
{
  size_t copied = 0;

  const std::lock_guard protect{lock};

  while ((copied < n) && !queued_data.empty()) {
    PCMData &current_pcm_data = queued_data.front();
    size_t current_available = current_pcm_data.size() - offset;
    size_t max = n - copied;
    if (current_available > max) {
      std::copy(current_pcm_data.data() + offset,
                current_pcm_data.data() + offset + max,
                buffer + copied);
      offset += max;
      return n;
    } else {
      std::copy(current_pcm_data.data() + offset,
                current_pcm_data.data() + offset + current_available,
                buffer + copied);
      copied += current_available;
      queued_data.pop_front();
      offset = 0;
    }
  }

  return copied;
}
