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

#ifndef XCSOAR_AUDIO_ALSA_PCM_PLAYER_HPP
#define XCSOAR_AUDIO_ALSA_PCM_PLAYER_HPP

#include "PCMPlayer.hpp"

#include "Compiler.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>

#include <boost/asio/posix/stream_descriptor.hpp>

#include <alsa/asoundlib.h>

namespace boost { namespace asio { class io_service; }}

/**
 * PCMPlayer implementation for the ALSA sound system
 */
class ALSAPCMPlayer : public PCMPlayer {
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

  boost::asio::io_service &io_service;

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

  static bool SetParameters(snd_pcm_t &alsa_handle, unsigned sample_rate,
                            bool big_endian_source, unsigned latency,
                            unsigned &channels);

public:
  explicit ALSAPCMPlayer(boost::asio::io_service &_io_service);
  virtual ~ALSAPCMPlayer();

  /* virtual methods from class PCMPlayer */
  bool Start(PCMDataSource &source) override;
  void Stop() override;
};

#endif
