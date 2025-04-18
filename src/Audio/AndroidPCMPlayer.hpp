// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMPlayer.hpp"
#include "thread/Mutex.hxx"
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
