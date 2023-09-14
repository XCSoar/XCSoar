// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PCMResourcePlayer.hpp"

#include "PCMPlayerFactory.hpp"

#include "LogFile.hpp"
#include "ResourceLoader.hpp"
#include "util/SpanCast.hxx"

#include <utility>

PCMResourcePlayer::PCMResourcePlayer() :
    player(PCMPlayerFactory::CreateInstance())
{
}

bool
PCMResourcePlayer::PlayResource(const TCHAR *resource_name)
{
  PCMBufferDataSource::PCMData pcm_data =
    FromBytesStrict<const PCMBufferDataSource::PCMData::value_type>(
          ResourceLoader::Load(resource_name, _T("WAVE")));
  if (pcm_data.data() == nullptr) {
    LogFormat(_T("PCM resource \"%s\" not found!"), resource_name);
    return false;
  }

  const std::lock_guard protect{lock};

  if (1 == buffer_data_source.Add(std::move(pcm_data))) {
    if (!player->Start(buffer_data_source)) {
      buffer_data_source.Clear();
      return false;
    }
  }

  return true;
}
