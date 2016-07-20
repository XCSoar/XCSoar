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

#ifndef XCSOAR_AUDIO_ANDROID_PCM_PLAYER_HPP
#define XCSOAR_AUDIO_ANDROID_PCM_PLAYER_HPP

#include "PCMPlayer.hpp"

#include "Thread/Mutex.hpp"
#include "SLES/Object.hpp"
#include "SLES/Play.hpp"
#include "SLES/AndroidSimpleBufferQueue.hpp"

/**
 * PCMPlayer implementation for Android, based on the OpenSL ES API
 */
class AndroidPCMPlayer : public PCMPlayer {
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

  void Enqueue();

public:
  AndroidPCMPlayer() = default;
  virtual ~AndroidPCMPlayer();

  /* virtual methods from class PCMPlayer */
  bool Start(PCMSynthesiser &source) override;
  void Stop() override;
};

#endif
