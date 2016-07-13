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

#ifndef XCSOAR_AUDIO_PCM_PLAYER_HPP
#define XCSOAR_AUDIO_PCM_PLAYER_HPP

#include "Compiler.h"

#ifdef ANDROID
#include "Thread/Mutex.hpp"
#include "SLES/Object.hpp"
#include "SLES/Play.hpp"
#include "SLES/AndroidSimpleBufferQueue.hpp"

#define PCMPLAYER_SYNTHESISER_ONLY
#elif defined(ENABLE_ALSA)
#include <list>
#include <memory>

#include <assert.h>

#include <boost/assert.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <alsa/asoundlib.h>

#define PCMPLAYER_REQUIRES_IO_SERVICE
#elif defined(ENABLE_SDL)
#include <SDL_audio.h>
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef PCMPLAYER_SYNTHESISER_ONLY
class PCMSynthesiser;
#else
class PCMDataSource;
#endif

/**
 * An audio player that plays synthesized 16 bit mono PCM data.  It is
 * being fed by a #PCMDataSource object that gets called when more
 * PCM samples are needed.
 */
class PCMPlayer {
#ifdef PCMPLAYER_REQUIRES_IO_SERVICE
  boost::asio::io_service &io_service;
#endif

#ifdef PCMPLAYER_SYNTHESISER_ONLY
  PCMSynthesiser *source = nullptr;
#else
  PCMDataSource *source = nullptr;
#endif

#if !defined(ANDROID) && !defined(WIN32)
  unsigned channels;
#endif

#ifdef ANDROID

  SLES::Object engine_object;

  SLES::Object mix_object;

  SLES::Object play_object;
  SLES::Play play;
  SLES::AndroidSimpleBufferQueue queue;

  /**
   * This mutex protects the attributes "next" and "filled".  It is
   * only needed while playback is launched, when the initial buffers
   * are being enqueued in the caller thread, while another thread may
   * invoke the registered callback.
   */
  Mutex mutex;

  /**
   * The index of the next buffer to be enqueued.
   */
  unsigned next;

  /**
   * Does the "next" buffer already contain synthesised samples?  This
   * can happen when PCMDataSource::GetData() has been called, but
   * the OpenSL/ES buffer queue was full.  The buffer will then be
   * postponed.
   */
  bool filled;

  /**
   * An array of buffers.  It's one more than being managed by
   * OpenSL/ES, and the one not enqueued (see attribute #next) will be
   * written to.
   */
  int16_t buffers[3][4096];

#elif defined(WIN32)
#elif defined(ENABLE_ALSA)
  static inline void CloseAlsaHandle(snd_pcm_t *handle) {
    if (nullptr != handle)
      BOOST_VERIFY(0 == snd_pcm_close(handle));
  }

  using AlsaHandleUniquePtr =
      std::unique_ptr<snd_pcm_t, decltype(&CloseAlsaHandle)>;

  static inline AlsaHandleUniquePtr MakeAlsaHandleUniquePtr(
      snd_pcm_t *handle = nullptr) {
    return AlsaHandleUniquePtr(handle, &CloseAlsaHandle);
  }

  AlsaHandleUniquePtr alsa_handle = MakeAlsaHandleUniquePtr();

  snd_pcm_uframes_t buffer_size;
  std::unique_ptr<int16_t[]> buffer;

  std::list<boost::asio::posix::stream_descriptor> read_poll_descs;
  std::list<boost::asio::posix::stream_descriptor> write_poll_descs;
  bool poll_descs_registered = false;

  void StartEventHandling();
  void StopEventHandling();

  static bool TryRecoverFromError(snd_pcm_t &alsa_handle, int error);
  bool TryRecoverFromError(int error) {
    assert(alsa_handle);
    return TryRecoverFromError(*alsa_handle, error);
  }

  static bool WriteFrames(snd_pcm_t &alsa_handle, int16_t *buffer,
                          size_t n, bool try_recover_on_error = true);
  bool WriteFrames(size_t n, bool try_recover_on_error = true) {
    assert(alsa_handle);
    assert(buffer);
    return WriteFrames(*alsa_handle, buffer.get(), n, try_recover_on_error);
  }

  bool OnEvent();

  void OnReadEvent(boost::asio::posix::stream_descriptor &fd,
                   const boost::system::error_code &ec);
  void OnWriteEvent(boost::asio::posix::stream_descriptor &fd,
                    const boost::system::error_code &ec);
#elif defined(ENABLE_SDL)
  SDL_AudioDeviceID device = -1;

  inline void AudioCallback(int16_t *stream, size_t len);
#endif

#ifdef ANDROID
  void Enqueue();
#elif defined(WIN32)
#else
  /**
   * Fill a buffer with data from the player's data source.
   * Then perform upmixing if necessary.
   * If the buffer could not be filled with the requested amount of frames
   * completely, the rest of the frames is filled with silence (zeros).
   * @param buffer The buffer which is filled. Must be at least n * channels
   * elements long.
   * @param n The number of mono frames which should be read.
   * @return The number of read frames (an upmixed mono frame counts as one),
   * not including the silence frames.
   */
  gcc_nonnull_all
  size_t FillPCMBuffer(int16_t *buffer, size_t n);

#endif

public:
#ifdef PCMPLAYER_REQUIRES_IO_SERVICE
  explicit PCMPlayer(boost::asio::io_service &_io_service);
#else
  PCMPlayer();
#endif
  ~PCMPlayer();

  /**
   * Start playback.
   *
   * @param source a PCMDataSource instance that will be used to
   * read sound; the caller is responsible for releasing it (not
   * before playback is stopped)
   */
#ifdef PCMPLAYER_SYNTHESISER_ONLY
  bool Start(PCMSynthesiser &source);
#else
  bool Start(PCMDataSource &source);
#endif

  /**
   * Stop playback and close the audio device.  This method is
   * synchronous.
   */
  void Stop();
};

#endif
