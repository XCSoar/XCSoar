// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once


#include "Features.hpp"

#if defined(HAVE_PCM_PLAYER) && !defined(ANDROID) && !defined(_WIN32)
class PCMResourcePlayer;

extern PCMResourcePlayer *pcm_resource_player;

void
InitialisePCMResourcePlayer();

void
DeinitialisePCMResourcePlayer();
#else
static inline void
InitialisePCMResourcePlayer()
{
}

static inline void
DeinitialisePCMResourcePlayer()
{
}
#endif

class ScopeGlobalPCMResourcePlayer final {
public:
  ScopeGlobalPCMResourcePlayer() {
    InitialisePCMResourcePlayer();
  }

  ~ScopeGlobalPCMResourcePlayer() {
    DeinitialisePCMResourcePlayer();
  }
};
