// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SDLPCMPlayer.hpp"

#include "LogFile.hpp"
#include "PCMDataSource.hpp"

#include <SDL3/SDL_error.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#include "Apple/Services.hpp"
#endif

#include <algorithm>
#include <iterator>

SDLPCMPlayer::~SDLPCMPlayer()
{
  Stop();
}

bool
SDLPCMPlayer::Start(PCMDataSource &_source)
{
  const unsigned sample_rate = _source.GetSampleRate();
  const bool big_endian = _source.IsBigEndian();

  if (stream != nullptr &&
      (source->GetSampleRate() != sample_rate ||
       source->IsBigEndian() != big_endian))
    Stop();

  if (stream != nullptr) {
    /* The callback holds this same lock while using the source.  Do not
       resume the device under it: the device has its own lock. */
    SDL_LockAudioStream(stream);
    if (source != &_source || exhausted)
      SDL_ClearAudioStream(stream);
    source = &_source;
    exhausted = false;
    SDL_UnlockAudioStream(stream);
  } else {
    /* SDL converts our mono signed 16-bit samples to the device format,
       including float output on iOS. */
    const SDL_AudioFormat format = big_endian
      ? SDL_AUDIO_S16BE
      : SDL_AUDIO_S16LE;
    const SDL_AudioSpec spec{format, 1, int(sample_rate)};
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                      &spec, AudioCallback, this);
    if (stream == nullptr) {
      LogFmt("SDLPCMPlayer: SDL_OpenAudioDeviceStream failed: {}",
             SDL_GetError());
      return false;
    }

    /* SDL_OpenAudioDeviceStream opens paused, so the callback cannot run
       until both the source and the iOS audio session are ready. */
    channels = 1;
    source = &_source;
    exhausted = false;
  }

#if defined(__APPLE__) && TARGET_OS_IPHONE
  /* CoreAudio may reset the shared AVAudioSession when opening a device.
     Mark the vario active before resuming so one-shot sound effects do
     not deactivate its session. */
  SetAudioVarioSessionActive(true);
  ActivateAudioSession();
#endif

  if (!SDL_ResumeAudioStreamDevice(stream)) {
    LogFmt("SDLPCMPlayer: SDL_ResumeAudioStreamDevice failed: {}",
           SDL_GetError());
    Stop();
    return false;
  }

  return true;
}

void
SDLPCMPlayer::Stop()
{
  /* Destroying the stream closes its device and waits for its callback.
     Keep the source alive until that has completed. */
  if (stream != nullptr)
    SDL_DestroyAudioStream(stream);

  stream = nullptr;
  source = nullptr;

#if defined(__APPLE__) && TARGET_OS_IPHONE
  SetAudioVarioSessionActive(false);
#endif
}

void SDLCALL
SDLPCMPlayer::AudioCallback(void *ctx, SDL_AudioStream *stream,
                            int additional_amount,
                            [[maybe_unused]] int total_amount)
{
  auto &player = *static_cast<SDLPCMPlayer *>(ctx);
  if (additional_amount <= 0 || player.exhausted)
    return;

  /* Use a bounded scratch buffer even when SDL requests a large block.
     additional_amount is measured in our input format, not the device's. */
  int16_t buffer[4096];
  size_t remaining = (size_t(additional_amount) + sizeof(buffer[0]) - 1) /
    sizeof(buffer[0]);
  while (remaining > 0) {
    const size_t frames = std::min(remaining, std::size(buffer));
    const size_t n = player.FillPCMBuffer(buffer, frames);
    if (n > 0 &&
        !SDL_PutAudioStreamData(stream, buffer, int(n * sizeof(buffer[0]))))
      return;

    if (n < frames) {
      /* Let the last samples drain; pausing here would cut off the tail.
         SDL supplies silence afterwards until Start() or Stop(). */
      player.exhausted = true;
      SDL_FlushAudioStream(stream);
      return;
    }
    remaining -= frames;
  }
}
