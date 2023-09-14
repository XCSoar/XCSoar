// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ANDROID
#define PCMPLAYER_SYNTHESISER_ONLY
#endif

#if !defined(ANDROID) && !defined(_WIN32)

#include <cstddef>
#include <cstdint>

#endif

#ifdef PCMPLAYER_SYNTHESISER_ONLY
class PCMSynthesiser;
#else
class PCMDataSource;
#endif

/**
 * Abstract base class for audio players that play 16 bit mono PCM data.  It is
 * being fed by a #PCMDataSource object that gets called when more
 * PCM samples are needed.
 */
class PCMPlayer {
public:
  /**
   * Start playback.
   *
   * @param source a PCMDataSource instance that will be used to
   * read sound; the caller is responsible for releasing it (not
   * before playback is stopped)
   */
#ifdef PCMPLAYER_SYNTHESISER_ONLY
  virtual bool Start(PCMSynthesiser &source) = 0;
#else
  virtual bool Start(PCMDataSource &source) = 0;
#endif

  /**
   * Stop playback and close the audio device.  This method is
   * synchronous.
   */
  virtual void Stop() = 0;

  virtual ~PCMPlayer() {}

protected:
#ifdef PCMPLAYER_SYNTHESISER_ONLY
  PCMSynthesiser *source = nullptr;
#else
  PCMDataSource *source = nullptr;
#endif

#if !defined(ANDROID) && !defined(_WIN32)
  unsigned channels;

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
  [[gnu::nonnull]]
  size_t FillPCMBuffer(int16_t *buffer, size_t n);
#endif
};
