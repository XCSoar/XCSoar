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


#include "PCMMixer.hpp"

#include "LogFile.hpp"

#include <tchar.h>

#include <utility>


PCMMixer::PCMMixer(unsigned _sample_rate,
                   std::unique_ptr<PCMPlayer> &&_player) :
  mixer_data_source(_sample_rate),
  player(std::move(_player))
{
}

bool
PCMMixer::Start(PCMDataSource &source)
{
  const unsigned src_sample_rate = source.GetSampleRate();
  const unsigned mixer_sample_rate = mixer_data_source.GetSampleRate();

  if (src_sample_rate != mixer_sample_rate) {
    /* Resampling is not supported yet */
    LogFormat(_T("Cannot playback PCM data source with sample rate of %u Hz, ")
                 _T("because the mixer sample rate is %u Hz"),
              src_sample_rate,
              mixer_sample_rate);
    return false;
  }

  const ScopeLock protect(lock);

  if (!mixer_data_source.AddSource(source)) {
    LogFormat(_T("Cannot handle PCM data source to mixer, because the mixer ")
                  _T("capacity is exceeded"));
    return false;
  }

  if (!player->Start(mixer_data_source)) {
    mixer_data_source.RemoveSource(source);
    return false;
  }

  return true;
}

void
PCMMixer::Stop(PCMDataSource &source)
{
  const ScopeLock protect(lock);
  mixer_data_source.RemoveSource(source);
}
