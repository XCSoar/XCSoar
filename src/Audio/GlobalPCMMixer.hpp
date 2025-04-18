// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once


#include "Features.hpp"

class EventLoop;

#ifdef HAVE_PCM_MIXER
class PCMMixer;

extern PCMMixer *pcm_mixer;

void
InitialisePCMMixer(EventLoop &event_loop);

void
DeinitialisePCMMixer();
#else
static inline void
InitialisePCMMixer(EventLoop &)
{
}

static inline void
DeinitialisePCMMixer()
{
}
#endif

class ScopeGlobalPCMMixer final {
public:
  ScopeGlobalPCMMixer(EventLoop &event_loop) {
    InitialisePCMMixer(event_loop);
  }

  ~ScopeGlobalPCMMixer() {
    DeinitialisePCMMixer();
  }
};
