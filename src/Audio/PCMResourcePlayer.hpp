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

#include "PCMBufferDataSource.hpp"
#include "PCMDataSource.hpp"
#include "PCMPlayer.hpp"
#include "Thread/Mutex.hpp"

#include <tchar.h>

#include <memory>

/**
 * Can play sounds, stored as raw PCM in an embedded resource, using #PCMPlayer
 */
class PCMResourcePlayer {
  Mutex lock;

  std::unique_ptr<PCMPlayer> player;

  PCMBufferDataSource buffer_data_source;

public:
  PCMResourcePlayer();
  virtual ~PCMResourcePlayer() = default;

  PCMResourcePlayer(PCMResourcePlayer &) = delete;
  PCMResourcePlayer &operator=(PCMResourcePlayer &) = delete;

  bool PlayResource(const TCHAR *resource_name);
};

#endif
