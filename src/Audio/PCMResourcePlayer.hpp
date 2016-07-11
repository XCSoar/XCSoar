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

#ifndef XCSOAR_AUDIO_PCM_RESOURCE_PLAYER_HPP
#define XCSOAR_AUDIO_PCM_RESOURCE_PLAYER_HPP

#include "PCMDataSource.hpp"
#include "PCMPlayer.hpp"
#include "Util/ConstBuffer.hxx"
#include "Thread/Mutex.hpp"

#include <tchar.h>
#include <stdint.h>

#include <list>
#include <memory>

/**
 * Can play sounds, stored as raw PCM in an embedded resource, using #PCMPlayer
 */
class PCMResourcePlayer : private PCMDataSource {
  typedef ConstBuffer<int16_t> PCMData;

  Mutex lock;

  std::unique_ptr<PCMPlayer> player;

  std::list<PCMData> queued_data;
  unsigned offset;

  /* virtual methods from class PCMDataSource */
  bool IsBigEndian() const override {
    return false; // Our PCM resources are always little endian
  }

  unsigned GetSampleRate() const override {
    return 44100; // Our PCM resources have this sample rate
  }

  size_t GetData(int16_t *buffer, size_t n) override;

public:
  PCMResourcePlayer();
  virtual ~PCMResourcePlayer() = default;

  PCMResourcePlayer(PCMResourcePlayer &) = delete;
  PCMResourcePlayer &operator=(PCMResourcePlayer &) = delete;

  bool PlayResource(const TCHAR *resource_name);
};

#endif
