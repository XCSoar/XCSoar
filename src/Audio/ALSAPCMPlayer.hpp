/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "PCMPlayer.hpp"
#include "event/SocketEvent.hxx"
#include "util/Compiler.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <memory>
#include <alsa/asoundlib.h>

/**
 * PCMPlayer implementation for the ALSA sound system
 */
class ALSAPCMPlayer : public PCMPlayer {
  static inline void CloseAlsaHandle(snd_pcm_t *handle) {
    if (nullptr != handle)
      snd_pcm_close(handle);
  }

  using AlsaHandleUniquePtr =
      std::unique_ptr<snd_pcm_t, decltype(&CloseAlsaHandle)>;

  static inline AlsaHandleUniquePtr MakeAlsaHandleUniquePtr(
      snd_pcm_t *handle = nullptr) {
    return AlsaHandleUniquePtr(handle, &CloseAlsaHandle);
  }

  AlsaHandleUniquePtr alsa_handle = MakeAlsaHandleUniquePtr();

  EventLoop &event_loop;

  snd_pcm_uframes_t buffer_size;
  std::unique_ptr<int16_t[]> buffer;

  std::forward_list<SocketEvent> poll_events;

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

  static bool SetParameters(snd_pcm_t &alsa_handle, unsigned sample_rate,
                            bool big_endian_source, unsigned latency,
                            unsigned &channels);

public:
  explicit ALSAPCMPlayer(EventLoop &event_loop) noexcept;
  virtual ~ALSAPCMPlayer();

  /* virtual methods from class PCMPlayer */
  bool Start(PCMDataSource &source) override;
  void Stop() override;

private:
  void OnSocketReady(unsigned events) noexcept;
};
