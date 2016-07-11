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

#include "PCMResourcePlayer.hpp"

#include "PCMPlayerFactory.hpp"

#include "Compiler.h"
#include "LogFile.hpp"
#include "ResourceLoader.hpp"

#ifdef ENABLE_ALSA
#include "IO/Async/AsioThread.hpp"
#include "IO/Async/GlobalAsioThread.hpp"
#endif

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <tchar.h>

#include <algorithm>


PCMResourcePlayer::PCMResourcePlayer() :
  player(PCMPlayerFactory::CreateInstance(
#ifdef ENABLE_ALSA
      asio_thread->Get()
#endif
      ))
{
}

size_t
PCMResourcePlayer::GetData(int16_t *buffer, size_t n)
{
  size_t copied = 0;

  const ScopeLock protect(lock);

  while ((copied < n) && !queued_data.empty()) {
    PCMData &current_pcm_data = queued_data.front();
    size_t current_available = current_pcm_data.size - offset;
    size_t max = n - copied;
    if (current_available > max) {
      std::copy(current_pcm_data.data + offset,
                current_pcm_data.data + offset + max,
                buffer + copied);
      offset += max;
      return n;
    } else {
      std::copy(current_pcm_data.data + offset,
                current_pcm_data.data + offset + current_available,
                buffer + copied);
      copied += current_available;
      queued_data.pop_front();
      offset = 0;
    }
  }

  return copied;
}

bool
PCMResourcePlayer::PlayResource(const TCHAR *resource_name)
{
  PCMData pcm_data =
      PCMData::FromVoid(ResourceLoader::Load(resource_name, _T("WAVE")));
  if (pcm_data.IsNull()) {
    LogFormat(_T("PCM resource \"%s\" not found!"), resource_name);
    return false;
  }

  const ScopeLock protect(lock);

  queued_data.push_back(pcm_data);
  if (1 == queued_data.size()) {
    offset = 0;
    if (!player->Start(*this)) {
      queued_data.clear();
      offset = 0;
      return false;
    }
    return true;
  }

  return true;
}
