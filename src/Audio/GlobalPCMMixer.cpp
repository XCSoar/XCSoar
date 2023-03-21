// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlobalPCMMixer.hpp"
#include "PCMPlayerFactory.hpp"
#include "PCMMixer.hpp"

#include <cassert>
#include <memory>

PCMMixer *pcm_mixer = nullptr;

void
InitialisePCMMixer(EventLoop &event_loop)
{
  assert(nullptr == pcm_mixer);

  pcm_mixer =
      new PCMMixer(44100,
                   std::unique_ptr<PCMPlayer>(PCMPlayerFactory::CreateInstanceForDirectAccess(event_loop)));
}

void
DeinitialisePCMMixer()
{
  assert(nullptr != pcm_mixer);

  delete pcm_mixer;
  pcm_mixer = nullptr;
}
