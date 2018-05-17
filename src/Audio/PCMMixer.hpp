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

#ifndef XCSOAR_AUDIO_PCM_MIXER_HPP
#define XCSOAR_AUDIO_PCM_MIXER_HPP

#include "PCMPlayer.hpp"

#include "PCMMixerDataSource.hpp"

#include "Thread/Mutex.hpp"

#include <memory>

/**
 * Mixes PCM data from multiple #PCMDataSource instances, using a
 * #PCMMixerDataSource, to be played simultaneously by one #PCMPlayer.
 *
 * All public functions are thread-safe.
 */
class PCMMixer final {
  Mutex lock;

  PCMMixerDataSource mixer_data_source;

  std::unique_ptr<PCMPlayer> player;

public:
  PCMMixer(unsigned sample_rate, std::unique_ptr<PCMPlayer> &&player);
  virtual ~PCMMixer() = default;

  PCMMixer(PCMMixer &) = delete;
  PCMMixer &operator=(PCMMixer &) = delete;

  /**
   * Start playback for a given source
   */
  bool Start(PCMDataSource &source);

  /**
   * Stop playback for a given source
   */
  void Stop(PCMDataSource &source);

  void SetVolume(unsigned vol_percent) {
    mixer_data_source.SetVolume(vol_percent);
  }

  static constexpr unsigned GetMaxSafeVolume() {
    return decltype(mixer_data_source)::GetMaxSafeVolume();
  }
};

#endif
