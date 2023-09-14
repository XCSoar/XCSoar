// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project


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

  const std::lock_guard protect{lock};

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
  const std::lock_guard protect{lock};
  mixer_data_source.RemoveSource(source);
}
