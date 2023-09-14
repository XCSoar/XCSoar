// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#if !defined(ANDROID) && !defined(_WIN32)

#include "PCMPlayer.hpp"

#include "PCMDataSource.hpp"
#include "AudioAlgorithms.hpp"

#include <cassert>

#include <algorithm>

size_t
PCMPlayer::FillPCMBuffer(int16_t *buffer, size_t n)
{
  assert(n > 0);
  assert(nullptr != source);
  const size_t n_read = source->GetData(buffer, n);
  if (n_read > 0)
    UpmixMonoPCM(buffer, n_read, channels);
  if (n_read < n)
    std::fill(buffer + n_read * channels, buffer + n * channels, 0);
  return n_read;
}

#endif
