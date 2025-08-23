// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>
#include <vector>

class SoundUtil {
public:
  static bool Play(const TCHAR *resource_name);

private:
  static std::vector<uint8_t> RawToWav(const uint8_t *rawData, size_t rawSize,
                                       int sampleRate = 44100,
                                       int bitsPerSample = 16,
                                       int channels = 1);
};
