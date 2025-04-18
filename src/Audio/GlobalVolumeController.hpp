// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

#ifdef HAVE_VOLUME_CONTROLLER
class VolumeController;

extern VolumeController *volume_controller;

void
InitialiseVolumeController();

void
DeinitialiseVolumeController();
#else
static inline void
InitialiseVolumeController()
{
}

static inline void
DeinitialiseVolumeController()
{
}
#endif

class ScopeGlobalVolumeController final {
public:
  ScopeGlobalVolumeController() {
    InitialiseVolumeController();
  }

  ~ScopeGlobalVolumeController() {
    DeinitialiseVolumeController();
  }
};
