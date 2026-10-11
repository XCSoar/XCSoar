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
PCMResourcePlayer::PlayResource(const char *resource_name)
{
  PCMBufferDataSource::PCMData pcm_data =
    FromBytesStrict<const PCMBufferDataSource::PCMData::value_type>(
          ResourceLoader::Load(resource_name, "WAVE"));
  if (pcm_data.data() == nullptr) {
    LogFormat("PCM resource not found: %s", resource_name);
    return false;
  }

  const std::lock_guard protect{lock};

  const unsigned queue_size = buffer_data_source.Add(std::move(pcm_data));
  if (1 == queue_size) {
    if (!player->Start(buffer_data_source)) {
      buffer_data_source.Clear();
      return false;
    }
  } else {
    /* Re-add buffer source to mixer so queued chunks are played after the
       mixer removed us when the previous chunk finished (GetData returned 0).
       See GitHub issue #2113. */
    if (!player->Start(buffer_data_source))
      LogFormat("PCMResourcePlayer: failed to re-add buffer source");
  }

  return true;
}

bool
PCMResourcePlayer::PlayResources(std::span<const char *const> resource_names)
{
  const std::lock_guard protect{lock};

  buffer_data_source.Clear();

  unsigned queue_size = 0;
  for (const char *resource_name : resource_names) {
    PCMBufferDataSource::PCMData pcm_data =
      FromBytesStrict<const PCMBufferDataSource::PCMData::value_type>(
            ResourceLoader::Load(resource_name, "WAVE"));
    if (pcm_data.data() == nullptr) {
      LogFormat("PCM resource not found: %s", resource_name);
      continue;
    }

    queue_size = buffer_data_source.Add(std::move(pcm_data));
  }

  if (queue_size == 0)
    return false;

  if (!player->Start(buffer_data_source)) {
    buffer_data_source.Clear();
    return false;
  }

  return true;
}
