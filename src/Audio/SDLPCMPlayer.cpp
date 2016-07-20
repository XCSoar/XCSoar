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

#include "SDLPCMPlayer.hpp"

#include "PCMDataSource.hpp"

#include <assert.h>

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
  wanted.format = AUDIO_S16SYS;
  wanted.channels = 1;
  wanted.samples = 4096;
  wanted.callback = [](void *ud, Uint8 *stream, int len_bytes) {
    assert(nullptr != ud);
    assert(nullptr != stream);
    assert(len_bytes > 0);

    reinterpret_cast<SDLPCMPlayer *>(ud)->AudioCallback(
        reinterpret_cast<int16_t *>(stream),
        static_cast<size_t>(len_bytes));
  };
  wanted.userdata = this;

  device = SDL_OpenAudioDevice(nullptr, 0, &wanted, &actual,
                               SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
  if (device < 1)
    return false;

  channels = static_cast<size_t>(actual.channels);

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
}

inline void
SDLPCMPlayer::AudioCallback(int16_t *stream, size_t len_bytes)
{
  const size_t num_frames = len_bytes / (channels * sizeof(stream[0]));
  const size_t read_frames = FillPCMBuffer(stream, num_frames);
  if (0 == read_frames)
    SDL_PauseAudioDevice(device, 1);
}
