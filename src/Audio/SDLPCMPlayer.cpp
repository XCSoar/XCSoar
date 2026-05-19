// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SDLPCMPlayer.hpp"

#include "LogFile.hpp"
#include "PCMDataSource.hpp"

#include <SDL_error.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#include <cassert>
#include <algorithm>

SDLPCMPlayer::~SDLPCMPlayer()
{
  Stop();
}

bool
SDLPCMPlayer::Start(PCMDataSource &_source)
{
  const unsigned new_sample_rate = _source.GetSampleRate();

  if ((nullptr != source) && (device > 0)) {
    if (source->GetSampleRate() == new_sample_rate) {
      /* already open, just change the synthesiser */
      SDL_LockAudioDevice(device);
      source = &_source;
      SDL_PauseAudioDevice(device, 0);
      SDL_UnlockAudioDevice(device);
    }

    Stop();
  }

  SDL_AudioSpec wanted, actual;
  wanted.freq = static_cast<int>(new_sample_rate);
#if defined(__APPLE__) && TARGET_OS_IPHONE
  wanted.format = AUDIO_F32SYS;
#else
  wanted.format = AUDIO_S16SYS;
#endif
  wanted.channels = 1;
  wanted.samples = 4096;
  wanted.callback = [](void *ud, Uint8 *stream, int len_bytes) {
    assert(nullptr != ud);
    assert(nullptr != stream);
    assert(len_bytes > 0);

    reinterpret_cast<SDLPCMPlayer *>(ud)->AudioCallback(
        stream,
        static_cast<size_t>(len_bytes));
  };
  wanted.userdata = this;

  device = SDL_OpenAudioDevice(nullptr, 0, &wanted, &actual,
                               SDL_AUDIO_ALLOW_CHANNELS_CHANGE |
                               SDL_AUDIO_ALLOW_FORMAT_CHANGE);
  if (device < 1) {
    LogFmt("SDLPCMPlayer: SDL_OpenAudioDevice failed: {}", SDL_GetError());
    return false;
  }

  LogFmt("SDLPCMPlayer: opened audio device rate={} format={} channels={}",
         actual.freq, actual.format, actual.channels);

  channels = static_cast<size_t>(actual.channels);
  format = actual.format;

  if (format != AUDIO_S16SYS && format != AUDIO_F32SYS) {
    LogFmt("SDLPCMPlayer: unsupported audio format {}", format);
    SDL_CloseAudioDevice(device);
    device = -1;
    return false;
  }

  if (format == AUDIO_F32SYS)
    convert_buffer.resize(actual.samples * channels);

  source = &_source;
  SDL_PauseAudioDevice(device, 0);

  return true;
}

void
SDLPCMPlayer::Stop()
{
  if (device > 0)
    SDL_CloseAudioDevice(device);

  device = -1;
  source = nullptr;
  convert_buffer.clear();
}

inline void
SDLPCMPlayer::AudioCallback(Uint8 *stream, size_t len_bytes)
{
  const size_t read_frames = format == AUDIO_F32SYS
    ? AudioCallback(reinterpret_cast<float *>(stream), len_bytes)
    : AudioCallback(reinterpret_cast<int16_t *>(stream), len_bytes);

  if (0 == read_frames)
    SDL_PauseAudioDevice(device, 1);
}

inline size_t
SDLPCMPlayer::AudioCallback(int16_t *stream, size_t len_bytes)
{
  const size_t num_frames = len_bytes / (channels * sizeof(stream[0]));
  const size_t read_frames = FillPCMBuffer(stream, num_frames);
  return read_frames;
}

inline size_t
SDLPCMPlayer::AudioCallback(float *stream, size_t len_bytes)
{
  const size_t num_frames = len_bytes / (channels * sizeof(stream[0]));
  const size_t num_samples = num_frames * channels;

  if (convert_buffer.size() < num_samples)
    convert_buffer.resize(num_samples);

  /* SDL/CoreAudio on iOS uses float output even though XCSoar's PCM pipeline
     produces signed 16-bit samples. */
  const size_t read_frames = FillPCMBuffer(convert_buffer.data(), num_frames);
  std::transform(convert_buffer.begin(), convert_buffer.begin() + num_samples,
                 stream, [](int16_t value) {
                   return (float)value / 32768.f;
                 });

  return read_frames;
}
