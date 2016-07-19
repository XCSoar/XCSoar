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

#ifndef XCSOAR_AUDIO_PCM_MIXER_DATA_SOURCE_HPP
#define XCSOAR_AUDIO_PCM_MIXER_DATA_SOURCE_HPP

#include "PCMDataSource.hpp"

#include "OS/ByteOrder.hpp"
#include "Thread/Mutex.hpp"


/**
 * #PCMDataSource implementation which mixes PCM data streams from multiple
 * other #PCMDataSource instances to one PCM data stream.
 *
 * Data sources with different byte order can be mixed, but the sample rate
 * must always be the same, because resampling is not supported yet.
 */
class PCMMixerDataSource : public PCMDataSource {
  static constexpr unsigned MAX_MIXER_SOURCES_COUNT = 2;

  const unsigned sample_rate;

  unsigned vol_percent = 100 / MAX_MIXER_SOURCES_COUNT;

  PCMDataSource *sources[MAX_MIXER_SOURCES_COUNT] = {};

  Mutex lock;

public:
  explicit PCMMixerDataSource(unsigned _sample_rate) :
      sample_rate(_sample_rate) {}

  virtual ~PCMMixerDataSource() = default;

  PCMMixerDataSource(PCMMixerDataSource &) = delete;
  PCMMixerDataSource &operator=(PCMMixerDataSource &) = delete;

  /**
   * Add a #PCMDataSource instance for mixing.
   *
   * @return false, if the capacity is exceeded. Otherwise true.
   */
  bool AddSource(PCMDataSource &source);

  /**
   * Remove a #PCMDataSource instance for mixing.
   */
  void RemoveSource(PCMDataSource &source);

  /**
   * Set volume level in percent. The volume for every source is reduced to
   * the given value.
   * When x sources are being played, up to 100/x% are safe, and clipping
   * artifacts can occur for higher levels, see GetMaxSafeVolume().
   */
  void SetVolume(unsigned vol_percent);

  /**
   * Get max volume value which is "safe". No clipping artifacts
   * can occur up to this volume level.
   */
  static constexpr unsigned GetMaxSafeVolume() {
    return 100 / MAX_MIXER_SOURCES_COUNT;
  }

  /* virtual methods from class PCMDataSource */
  bool IsBigEndian() const override {
    return ::IsBigEndian();
  }

  unsigned GetSampleRate() const override {
    return sample_rate;
  }

  size_t GetData(int16_t *buffer, size_t n) override;
};

#endif
