// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlobalPCMResourcePlayer.hpp"
#include "PCMResourcePlayer.hpp"

#include <cassert>

PCMResourcePlayer *pcm_resource_player = nullptr;

void
InitialisePCMResourcePlayer()
{
  assert(nullptr == pcm_resource_player);

  pcm_resource_player = new PCMResourcePlayer();
}

void
DeinitialisePCMResourcePlayer()
{
  assert(nullptr != pcm_resource_player);

  delete pcm_resource_player;
  pcm_resource_player = nullptr;
}
