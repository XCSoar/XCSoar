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

#include "AndroidPCMPlayer.hpp"

#include "PCMSynthesiser.hpp"

#include "SLES/Init.hpp"
#include "SLES/Engine.hpp"

#include "Util/Macros.hpp"
#include "LogFile.hpp"

#include <SLES/OpenSLES_Android.h>

AndroidPCMPlayer::~AndroidPCMPlayer()
{
  Stop();
}

bool
AndroidPCMPlayer::Start(PCMSynthesiser &_source)
{
  /* why, oh why is OpenSL/ES so complicated? */

  SLObjectItf _object;
  SLresult result = SLES::CreateEngine(&_object, 0, nullptr,
                                       0, nullptr, nullptr);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: slCreateEngine() result=%#x", (int)result);
    return false;
  }

  engine_object = SLES::Object(_object);

  result = engine_object.Realize(false);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: Engine.Realize() result=%#x", (int)result);
    engine_object.Destroy();
    return false;
  }

  SLEngineItf _engine;
  result = engine_object.GetInterface(*SLES::IID_ENGINE, &_engine);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: Engine.GetInterface(IID_ENGINE) result=%#x",
               (int)result);
    engine_object.Destroy();
    return false;
  }

  SLES::Engine engine(_engine);

  result = engine.CreateOutputMix(&_object, 0, nullptr, nullptr);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: CreateOutputMix() result=%#x", (int)result);
    engine_object.Destroy();
    return false;
  }

  mix_object = SLES::Object(_object);

  result = mix_object.Realize(false);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: Mix.Realize() result=%#x", (int)result);
    mix_object.Destroy();
    engine_object.Destroy();
    return false;
  }

  SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {
    SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
    ARRAY_SIZE(buffers) - 1,
  };

  SLDataFormat_PCM format_pcm;
  format_pcm.formatType = SL_DATAFORMAT_PCM;
  format_pcm.numChannels = 1;
  /* from the Android NDK docs: "Note that the field samplesPerSec is
     actually in units of milliHz, despite the misleading name." */
  format_pcm.samplesPerSec = _source.GetSampleRate() * 1000;
  format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
  format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
  format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
  format_pcm.endianness = _source.IsBigEndian() ?
      SL_BYTEORDER_BIGENDIAN : SL_BYTEORDER_LITTLEENDIAN;

  SLDataSource audioSrc = { &loc_bufq, &format_pcm };

  SLDataLocator_OutputMix loc_outmix = {
    SL_DATALOCATOR_OUTPUTMIX,
    mix_object,
  };

  SLDataSink audioSnk = {
    &loc_outmix,
    nullptr,
  };

  const SLInterfaceID ids2[] = {
    *SLES::IID_PLAY,
    *SLES::IID_ANDROIDSIMPLEBUFFERQUEUE,
  };
  static constexpr SLboolean req2[] = {
    SL_BOOLEAN_TRUE,
    SL_BOOLEAN_TRUE,
  };
  result = engine.CreateAudioPlayer(&_object, &audioSrc, &audioSnk,
                                    ARRAY_SIZE(ids2), ids2, req2);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: CreateAudioPlayer() result=%#x", (int)result);
    mix_object.Destroy();
    engine_object.Destroy();
    return false;
  }

  play_object = SLES::Object(_object);

  result = play_object.Realize(false);

  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: Play.Realize() result=%#x", (int)result);
    play_object.Destroy();
    mix_object.Destroy();
    engine_object.Destroy();
    return false;
  }

  SLPlayItf _play;
  result = play_object.GetInterface(*SLES::IID_PLAY, &_play);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: Play.GetInterface(IID_PLAY) result=%#x",
               (int)result);
    play_object.Destroy();
    mix_object.Destroy();
    engine_object.Destroy();
    return false;
  }

  play = SLES::Play(_play);

  SLAndroidSimpleBufferQueueItf _queue;
  result = play_object.GetInterface(*SLES::IID_ANDROIDSIMPLEBUFFERQUEUE,
                                    &_queue);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: Play.GetInterface(IID_ANDROIDSIMPLEBUFFERQUEUE) result=%#x",
               (int)result);
    play_object.Destroy();
    mix_object.Destroy();
    engine_object.Destroy();
    return false;
  }

  queue = SLES::AndroidSimpleBufferQueue(_queue);
  result = queue.RegisterCallback([](SLAndroidSimpleBufferQueueItf caller,
                                     void *pContext) {
      /**
       * OpenSL/ES callback which gets invoked when a buffer has been
       * consumed.  It synthesises and enqueues the next buffer.
       */
      reinterpret_cast<AndroidPCMPlayer *>(pContext)->Enqueue();
  }, this);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: Play.RegisterCallback() result=%#x", (int)result);
    play_object.Destroy();
    mix_object.Destroy();
    engine_object.Destroy();
    return false;
  }

  source = &_source;

  result = play.SetPlayState(SL_PLAYSTATE_PLAYING);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: Play.SetPlayState(PLAYING) result=%#x",
               (int)result);
    play_object.Destroy();
    mix_object.Destroy();
    engine_object.Destroy();
    source = nullptr;
    return false;
  }

  next = 0;
  filled = false;
  for (unsigned i = 0; i < ARRAY_SIZE(buffers) - 1; ++i)
    Enqueue();

  return true;
}

void
AndroidPCMPlayer::Stop()
{
  if (source == nullptr)
    return;

  play.SetPlayState(SL_PLAYSTATE_PAUSED);
  play_object.Destroy();
  mix_object.Destroy();
  engine_object.Destroy();

  source = nullptr;
}

void
AndroidPCMPlayer::Enqueue()
{
  assert(source != nullptr);

  ScopeLock protect(mutex);

  if (!filled) {
    filled = true;
    source->Synthesise(buffers[next], ARRAY_SIZE(buffers[next]));
  }

  SLresult result = queue.Enqueue(buffers[next], sizeof(buffers[next]));
  if (result == SL_RESULT_SUCCESS) {
    next = (next + 1) % ARRAY_SIZE(buffers);
    filled = false;
  }

  if (result != SL_RESULT_SUCCESS)
    LogFormat("PCMPlayer: Enqueue() result=%#x", (int)result);
}
