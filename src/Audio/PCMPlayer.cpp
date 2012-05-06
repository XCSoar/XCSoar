/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "PCMPlayer.hpp"
#include "PCMSynthesiser.hpp"

#ifdef ANDROID
#elif defined(WIN32)
#else
#include <SDL/SDL_audio.h>
#endif

#include <assert.h>

PCMPlayer::PCMPlayer():sample_rate(0), synthesiser(NULL) {}

PCMPlayer::~PCMPlayer()
{
  Stop();
}

#ifdef ANDROID
#elif defined(WIN32)
#else

static void
Synthesise(void *udata, Uint8 *stream, int len)
{
  PCMPlayer &player = *(PCMPlayer *)udata;

  const size_t num_frames = len / 4;
  int16_t *stereo = (int16_t *)(void *)stream;
  int16_t *mono = stereo + num_frames, *end = mono + num_frames;

  player.Synthesise(mono, num_frames);

  while (mono != end) {
    int16_t sample = *mono++;
    *stereo++ = sample;
    *stereo++ = sample;
  }
}

#endif

bool
PCMPlayer::Start(PCMSynthesiser &_synthesiser, unsigned _sample_rate)
{
#ifdef ANDROID
#elif defined(WIN32)
#else
  if (synthesiser != NULL) {
    if (_sample_rate == sample_rate) {
      /* already open, just change the synthesiser */
      SDL_LockAudio();
      synthesiser = &_synthesiser;
      SDL_UnlockAudio();
      return true;
    }

    Stop();
  }

  sample_rate = _sample_rate;

  SDL_AudioSpec spec;
  spec.freq = sample_rate;
  spec.format = AUDIO_S16SYS;
  spec.channels = 2;
  spec.samples = 4096;
  spec.callback = ::Synthesise;
  spec.userdata = this;

  if (SDL_OpenAudio(&spec, NULL) < 0)
    return false;

  synthesiser = &_synthesiser;
  SDL_PauseAudio(0);

  return true;
#endif
}

void
PCMPlayer::Stop()
{
#ifdef ANDROID
#elif defined(WIN32)
#else
  if (synthesiser == NULL)
    return;

  SDL_CloseAudio();
  sample_rate = 0;
  synthesiser = NULL;
#endif
}

#ifdef ANDROID
#elif defined(WIN32)
#else
void
PCMPlayer::Synthesise(void *buffer, size_t n)
{
  assert(synthesiser != NULL);

  synthesiser->Synthesise((int16_t *)buffer, n);
}
#endif
