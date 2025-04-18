// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"
#include "util/ByteOrder.hxx"

#include <algorithm>
#include <limits>
#include <type_traits>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

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

  for (size_t i = num_mono_frames; i-- > 0;) {
    std::fill(pcm_stream_buffer + i * num_channels,
              pcm_stream_buffer + i * num_channels + num_channels,
              pcm_stream_buffer[i]);
  }
}

/**
 * Convert an int32_t value to int16_t and perform clipping on
 * overflow / underflow.
 */
constexpr int16_t Clip(int32_t value) noexcept {
  return static_cast<int16_t>(
      std::clamp(
          value,
          static_cast<int32_t>(std::numeric_limits<int16_t>::min()),
          static_cast<int32_t>(std::numeric_limits<int16_t>::max())));
}

/**
 * Mix PCM data from a data source (which is read using the provided source
 * reader function) to a destination buffer (which already contains PCM data).
 *
 * The audio volume is lowered to the given percentage value.
 *
 * Performs cipping, if necessary.
 */
template<typename SrcReadFuncT>
inline void MixPCM(int16_t *dest, size_t num_frames,
                   unsigned vol_percent, SrcReadFuncT src_reader) {
  if (0 == vol_percent) {
    std::fill(dest, dest + num_frames, 0);
    return;
  }

  for (size_t i = 0; i < num_frames; ++i) {
    dest[i] += Clip(src_reader(i) * static_cast<int32_t>(vol_percent) / 100);
  }
}

/**
 * Mix PCM data from a given data source to a destination buffer
 * (which already contains PCM data).
 *
 * Performs cipping, if necessary.
 */
inline void MixPCM(int16_t *dest, const int16_t *src, size_t num_frames,
                   unsigned vol_percent) {
  MixPCM(dest, num_frames, vol_percent, [&src](size_t i) { return src[i]; });
}

/**
 * Mix PCM data from a given data source to a destination buffer
 * (which already contains PCM data). The data which is read from the source
 * buffer is and byte-swapped.
 *
 * Use this function, if the source is big endian and the destination
 * is little endian, or vice versa.
 *
 * Performs cipping, if necessary.
 */
inline void ByteSwapAndMixPCM(int16_t *dest, const int16_t *src,
                              size_t num_frames, unsigned vol_percent) {
  MixPCM(dest, num_frames, vol_percent,
         [&src](size_t i) {
           return static_cast<int32_t>(GenericByteSwap16(src[i]));
         });
}

/**
 * Lower audio volume of a PCM buffer to the given percentage value.
 */
inline void LowerVolume(int16_t *buffer, size_t num_frames,
                        unsigned vol_percent) {
  if (0 == vol_percent) {
    std::fill(buffer, buffer + num_frames, 0);
    return;
  }

  for (size_t i = 0; i < num_frames; ++i) {
    buffer[i] =
        static_cast<int16_t>(
            (static_cast<int32_t>(vol_percent) * buffer[i]) / 100);
  }
}

/**
 * Perform byte-swapping and lower audio volume of a PCM buffer to the given
 * percentage value.
 */
inline void ByteSwapAndLowerVolume(int16_t *buffer, size_t num_frames,
                                   unsigned vol_percent) {
  if (0 == vol_percent) {
    std::fill(buffer, buffer + num_frames, 0);
    return;
  }

  for (size_t i = 0; i < num_frames; ++i) {
    buffer[i] =
        static_cast<int16_t>(
            static_cast<int32_t>(vol_percent) *
                static_cast<int32_t>(GenericByteSwap16(buffer[i])) /
                100);
  }
}
