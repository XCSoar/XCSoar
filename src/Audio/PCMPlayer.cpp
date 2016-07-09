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

#include "PCMPlayer.hpp"
#include "PCMSynthesiser.hpp"
#include "Util/Macros.hpp"
#include "LogFile.hpp"

#ifdef ANDROID
#include "SLES/Init.hpp"
#include "SLES/Engine.hpp"

#include <SLES/OpenSLES_Android.h>
#elif defined(WIN32)
#elif defined(ENABLE_ALSA)
#include "IO/Async/AsioUtil.hpp"
#include "OS/ByteOrder.hpp"
#include "Util/NumberParser.hpp"

#include <alsa/asoundlib.h>
#elif defined(ENABLE_SDL)
#include "AudioAlgorithms.hpp"
#endif

#include <assert.h>

#ifdef ENABLE_ALSA
static constexpr char ALSA_DEVICE_ENV[] = "ALSA_DEVICE";
static constexpr char ALSA_LATENCY_ENV[] = "ALSA_LATENCY";

static constexpr char DEFAULT_ALSA_DEVICE[] = "default";
static constexpr unsigned DEFAULT_ALSA_LATENCY = 100000;
#endif

#ifdef PCMPLAYER_REQUIRES_IO_SERVICE
PCMPlayer::PCMPlayer(boost::asio::io_service &_io_service) :
  io_service(_io_service),
#else
PCMPlayer::PCMPlayer() :
#endif
  sample_rate(0), synthesiser(nullptr) {}

PCMPlayer::~PCMPlayer()
{
  Stop();
}

#ifdef ANDROID

#elif defined(WIN32)
#elif defined(ENABLE_ALSA)
void
PCMPlayer::StartEventHandling()
{
  if (!poll_descs_registered) {
    for (auto &fd : read_poll_descs) {
      fd.async_read_some(boost::asio::null_buffers(),
                          std::bind(&PCMPlayer::OnReadEvent,
                                    this,
                                    std::ref(fd),
                                    std::placeholders::_1));
    }

    for (auto &fd : write_poll_descs) {
      fd.async_write_some(boost::asio::null_buffers(),
                          std::bind(&PCMPlayer::OnWriteEvent,
                                    this,
                                    std::ref(fd),
                                    std::placeholders::_1));
    }

    poll_descs_registered = true;
  }
}

void
PCMPlayer::StopEventHandling()
{
  if (poll_descs_registered) {
    for (auto &sd : read_poll_descs) {
      sd.cancel();
    }

    for (auto &sd : write_poll_descs) {
      sd.cancel();
    }

    poll_descs_registered = false;
  }
}

void
PCMPlayer::OnEvent()
{
  snd_pcm_sframes_t n = snd_pcm_avail_update(alsa_handle.get());
  if (n < 0) {
    if (-EPIPE == n)
      LogFormat("ALSA PCM buffer underrun");
    else if ((-EINTR == n) || (-ESTRPIPE == n))
      LogFormat("ALSA PCM error: %s - trying to recover",
                snd_strerror(static_cast<int>(n)));
    else {
      // snd_pcm_recover() can only handle EPIPE, EINTR and ESTRPIPE
      LogFormat("Unrecoverable ALSA PCM error: %s",
                snd_strerror(static_cast<int>(n)));
      return;
    }

    if (0 == snd_pcm_recover(alsa_handle.get(), static_cast<int>(n), 1)) {
      LogFormat("ALSA PCM successfully recovered");
    }
    n = static_cast<snd_pcm_sframes_t>(buffer_size);
  }

  if (n > 0) {
    Synthesise(buffer.get(), static_cast<size_t>(n));
    BOOST_VERIFY(n == snd_pcm_writei(alsa_handle.get(),
                                     buffer.get(),
                                     static_cast<snd_pcm_uframes_t>(n)));
  }
}

void
PCMPlayer::OnReadEvent(boost::asio::posix::stream_descriptor &fd,
                       const boost::system::error_code &ec) {
  if (ec == boost::asio::error::operation_aborted)
    return;

  OnEvent();

  fd.async_read_some(boost::asio::null_buffers(),
                     std::bind(&PCMPlayer::OnReadEvent,
                               this,
                               std::ref(fd),
                               std::placeholders::_1));
}

void
PCMPlayer::OnWriteEvent(boost::asio::posix::stream_descriptor &fd,
                        const boost::system::error_code &ec) {
  if (ec == boost::asio::error::operation_aborted)
    return;

  OnEvent();

  fd.async_write_some(boost::asio::null_buffers(),
                      std::bind(&PCMPlayer::OnWriteEvent,
                                this,
                                std::ref(fd),
                                std::placeholders::_1));
}
#elif defined(ENABLE_SDL)

inline void
PCMPlayer::AudioCallback(int16_t *stream, size_t len_bytes)
{
  const size_t num_frames = len_bytes / (channels * sizeof(stream[0]));
  Synthesise(stream, num_frames);
}

#endif

bool
PCMPlayer::Start(PCMSynthesiser &_synthesiser, unsigned _sample_rate)
{
#ifdef ANDROID

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
  format_pcm.samplesPerSec = _sample_rate * 1000;
  format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
  format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
  format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
  format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN; // XXX

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
      reinterpret_cast<PCMPlayer *>(pContext)->Enqueue();
  }, this);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: Play.RegisterCallback() result=%#x", (int)result);
    play_object.Destroy();
    mix_object.Destroy();
    engine_object.Destroy();
    return false;
  }

  synthesiser = &_synthesiser;

  result = play.SetPlayState(SL_PLAYSTATE_PLAYING);
  if (result != SL_RESULT_SUCCESS) {
    LogFormat("PCMPlayer: Play.SetPlayState(PLAYING) result=%#x",
               (int)result);
    play_object.Destroy();
    mix_object.Destroy();
    engine_object.Destroy();
    synthesiser = nullptr;
    return false;
  }

  next = 0;
  filled = false;
  for (unsigned i = 0; i < ARRAY_SIZE(buffers) - 1; ++i)
    Enqueue();

  return true;
#elif defined(WIN32)
#elif defined(ENABLE_ALSA)
  if ((nullptr != synthesiser) && alsa_handle && (_sample_rate == sample_rate)) {
    /* just change the synthesiser */
    DispatchWait(io_service, [this, &_synthesiser]() {
      assert(alsa_handle);
      BOOST_VERIFY(0 == snd_pcm_drop(alsa_handle.get()));
      synthesiser = &_synthesiser;
      Synthesise(buffer.get(), buffer_size);
      BOOST_VERIFY(static_cast<snd_pcm_sframes_t>(buffer_size)
                       == snd_pcm_writei(alsa_handle.get(),
                                         buffer.get(),
                                         buffer_size));
    });
    return true;
  }

  Stop();

  assert(!alsa_handle);

  AlsaHandleUniquePtr new_alsa_handle = MakeAlsaHandleUniquePtr();
  {
    /* The "default" alsa device is normally the dmix plugin, or PulseAudio.
     * Some users might want to explicitly specify a hw (or plughw) device
     * for reduced latency. */
    const char *alsa_device = getenv(ALSA_DEVICE_ENV);
    if ((nullptr == alsa_device) || ('\0' == *alsa_device))
      alsa_device = DEFAULT_ALSA_DEVICE;
    LogFormat("Using ALSA PCM device %s (use environment variable "
                  "%s to override)",
              alsa_device, ALSA_DEVICE_ENV);

    snd_pcm_t *raw_alsa_handle;
    int alsa_error = snd_pcm_open(&raw_alsa_handle, alsa_device,
                                  SND_PCM_STREAM_PLAYBACK, 0);
    if (alsa_error < 0) {
      LogFormat("snd_pcm_open(0x%p, %s, SND_PCM_STREAM_PLAYBACK, 0) failed: %s",
                &alsa_handle, alsa_device, snd_strerror(alsa_error));
      return false;
    }
    new_alsa_handle = MakeAlsaHandleUniquePtr(raw_alsa_handle);
    assert(new_alsa_handle);
  }

  /* With the latency parameter in snd_pcm_set_params(), ALSA determines
   * buffer size and period time values to achieve this. We always want low
   * latency. But lower latency values result in a smaller buffer size,
   * more frequent interrupts, and a higher risk for buffer underruns. */
  unsigned latency = DEFAULT_ALSA_LATENCY;
  const char *latency_env_value = getenv(ALSA_LATENCY_ENV);
  if ((nullptr == latency_env_value) || ('\0' == *latency_env_value)) {
    latency = DEFAULT_ALSA_LATENCY;
  } else {
    char *p;
    latency = ParseUnsigned(latency_env_value, &p);
    if (*p != '\0') {
      LogFormat("Invalid %s value \"%s\"", ALSA_LATENCY_ENV, latency_env_value);
      return false;
    }
  }
  LogFormat("Using ALSA PCM latency %u μs (use environment variable "
                "%s to override)", latency, ALSA_LATENCY_ENV);

  int alsa_error = snd_pcm_set_params(new_alsa_handle.get(),
                                      IsLittleEndian()
                                          ? SND_PCM_FORMAT_S16_LE
                                          : SND_PCM_FORMAT_S16_BE,
                                      SND_PCM_ACCESS_RW_INTERLEAVED,
                                      1,
                                      _sample_rate,
                                      1,
                                      latency);
  if (alsa_error < 0) {
    LogFormat("snd_pcm_set_params(0x%p, %s, SND_PCM_ACCESS_RW_INTERLEAVED, 1, "
                  "%u, 1, %u) failed: %d - %s",
              new_alsa_handle.get(),
              IsLittleEndian()
                  ? "SND_PCM_FORMAT_S16_LE" : "SND_PCM_FORMAT_S16_BE",
              _sample_rate,
              latency,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  snd_pcm_sframes_t n = snd_pcm_avail(new_alsa_handle.get());
  if (n <= 0) {
    LogFormat("snd_pcm_avail(0x%p) failed: %ld - %s",
              new_alsa_handle.get(),
              static_cast<long>(n),
              snd_strerror(static_cast<int>(n)));
    return false;
  }

  buffer_size = static_cast<snd_pcm_uframes_t>(n);
  buffer = std::unique_ptr<int16_t[]>(new int16_t[buffer_size]);

  /* Why does Boost.Asio make it so hard to register a set of of standard
     poll() descriptors (struct pollfd)? */
  int poll_fds_count = snd_pcm_poll_descriptors_count(new_alsa_handle.get());
  if (poll_fds_count < 1) {
    LogFormat("snd_pcm_poll_descriptors_count(0x%p) returned %d",
              new_alsa_handle.get(),
              poll_fds_count);
    return false;
  }

  std::unique_ptr<struct pollfd[]> poll_fds(
      new struct pollfd[poll_fds_count]);
  BOOST_VERIFY(
      poll_fds_count ==
          snd_pcm_poll_descriptors(new_alsa_handle.get(),
                                   poll_fds.get(),
                                   static_cast<unsigned>(poll_fds_count)));
  assert(read_poll_descs.empty());
  assert(write_poll_descs.empty());
  for (int i = 0; i < poll_fds_count; ++i) {
    if ((poll_fds[i].events & POLLIN) || (poll_fds[i].events & POLLPRI)) {
      read_poll_descs.emplace_back(
          boost::asio::posix::stream_descriptor(io_service, poll_fds[i].fd));
    }
    if (poll_fds[i].events & POLLOUT) {
      write_poll_descs.emplace_back(
          boost::asio::posix::stream_descriptor(io_service, poll_fds[i].fd));
    }
  }

  synthesiser = &_synthesiser;
  Synthesise(buffer.get(), buffer_size);

  BOOST_VERIFY(n == snd_pcm_writei(new_alsa_handle.get(),
                                   buffer.get(),
                                   buffer_size));

  alsa_handle = std::move(new_alsa_handle);

  StartEventHandling();

  return true;
#else
  if ((nullptr != synthesiser) && (device > 0)) {
    if (_sample_rate == sample_rate) {
      /* already open, just change the synthesiser */
      SDL_LockAudioDevice(device);
      synthesiser = &_synthesiser;
      SDL_UnlockAudioDevice(device);
      return true;
    }

    Stop();
  }

  SDL_AudioSpec wanted, actual;
  wanted.freq = sample_rate;
  wanted.format = AUDIO_S16SYS;
  wanted.channels = 1;
  wanted.samples = 4096;
  wanted.callback = [](void *ud, Uint8 *stream, int len_bytes) {
    assert(nullptr != ud);
    assert(nullptr != stream);
    assert(len_bytes > 0);

    reinterpret_cast<PCMPlayer *>(ud)->AudioCallback(
        reinterpret_cast<int16_t *>(stream),
        static_cast<size_t>(len_bytes));
  };
  wanted.userdata = this;

  device = SDL_OpenAudioDevice(nullptr, 0, &spec, &actual,
                               SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
  if (device < 1)
    return false;

  channels = static_cast<size_t>(actual.channels);
  sample_rate = static_cast<unsigned>(actual.freq);

  synthesiser = &_synthesiser;
  SDL_PauseAudioDevice(device, 0);

  return true;
#endif
}

void
PCMPlayer::Stop()
{
#ifdef ANDROID
  if (synthesiser == nullptr)
    return;

  play.SetPlayState(SL_PLAYSTATE_PAUSED);
  play_object.Destroy();
  mix_object.Destroy();
  engine_object.Destroy();

  sample_rate = 0;
  synthesiser = nullptr;
#elif defined(WIN32)
#elif defined(ENABLE_ALSA)
  if ((nullptr != alsa_handle) || poll_descs_registered) {
    DispatchWait(io_service, [&]() {
      StopEventHandling();

      if (nullptr != alsa_handle) {
        BOOST_VERIFY(0 == snd_pcm_drop(alsa_handle.get()));
        alsa_handle.reset();
      }
    });
  }

  read_poll_descs.clear();
  write_poll_descs.clear();

  sample_rate = 0;
  synthesiser = nullptr;
#else
  if (device > 0)
    SDL_CloseAudioDevice(device);

  device = -1;
  sample_rate = 0;
  synthesiser = nullptr;
#endif
}

#ifdef ANDROID

void
PCMPlayer::Enqueue()
{
  assert(synthesiser != nullptr);

  ScopeLock protect(mutex);

  if (!filled) {
    filled = true;
    synthesiser->Synthesise(buffers[next], ARRAY_SIZE(buffers[next]));
  }

  SLresult result = queue.Enqueue(buffers[next], sizeof(buffers[next]));
  if (result == SL_RESULT_SUCCESS) {
    next = (next + 1) % ARRAY_SIZE(buffers);
    filled = false;
  }

  if (result != SL_RESULT_SUCCESS)
    LogFormat("PCMPlayer: Enqueue() result=%#x", (int)result);
}

#elif defined(WIN32)
#else

void
PCMPlayer::Synthesise(void *buffer, size_t n)
{
  assert(synthesiser != nullptr);

  synthesiser->Synthesise((int16_t *)buffer, n);
#ifdef ENABLE_SDL
  UpmixMonoPCM(reinterpret_cast<int16_t *>(buffer), n, channels);
#endif
}

#endif
