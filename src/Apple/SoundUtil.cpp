// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SoundUtil.hpp"
#include "LogFile.hpp"
#include "ResourceLoader.hpp"

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

static AVAudioPlayer *player = nil;

bool
SoundUtil::Play(const TCHAR *resource_name)
{
  auto data = ResourceLoader::Load(resource_name, _T("WAVE"));
  if (data.empty()) {
    LogFormat("Audio resource not found or empty: %s", resource_name);
    return false;
  }

  std::vector<uint8_t> wavData = RawToWav(reinterpret_cast<const uint8_t *>(data.data()), data.size());

  NSError *error = nil;
  player = [[AVAudioPlayer alloc] initWithData:[NSData dataWithBytes:wavData.data() length:wavData.size()] error:&error];

  if (!player) {
    if (error) {
      LogFormat("Failed to create AVAudioPlayer: %s", [[error localizedDescription] UTF8String]);
    } else {
      LogFormat("Failed to create AVAudioPlayer for unknown reason");
    }
    return false;
  }

  [player prepareToPlay];
  [player play];

  return true;
}

std::vector<uint8_t>
SoundUtil::RawToWav(const uint8_t *rawData, size_t rawSize, int sampleRate,
                    int bitsPerSample, int channels)
{
  std::vector<uint8_t> wav;
  int byteRate = sampleRate * channels * bitsPerSample / 8;
  int blockAlign = channels * bitsPerSample / 8;
  uint32_t dataChunkSize = static_cast<uint32_t>(rawSize);
  uint32_t riffChunkSize = 36 + dataChunkSize;

  wav.resize(44 + rawSize);

  uint8_t *p = wav.data();

  memcpy(p, "RIFF", 4);
  p += 4;
  *reinterpret_cast<uint32_t *>(p) = riffChunkSize;
  p += 4;
  memcpy(p, "WAVE", 4);
  p += 4;

  memcpy(p, "fmt ", 4);
  p += 4;
  *reinterpret_cast<uint32_t *>(p) = 16;
  p += 4;
  *reinterpret_cast<uint16_t *>(p) = 1;
  p += 2;
  *reinterpret_cast<uint16_t *>(p) = channels;
  p += 2;
  *reinterpret_cast<uint32_t *>(p) = sampleRate;
  p += 4;
  *reinterpret_cast<uint32_t *>(p) = byteRate;
  p += 4;
  *reinterpret_cast<uint16_t *>(p) = blockAlign;
  p += 2;
  *reinterpret_cast<uint16_t *>(p) = bitsPerSample;
  p += 2;

  memcpy(p, "data", 4);
  p += 4;
  *reinterpret_cast<uint32_t *>(p) = dataChunkSize;
  p += 4;
  memcpy(p, rawData, rawSize);

  return wav;
}
