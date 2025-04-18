// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlobalVolumeController.hpp"
#include "VolumeController.hpp"

#include <cassert>

VolumeController *volume_controller = nullptr;

void
InitialiseVolumeController()
{
  assert(nullptr == volume_controller);

  volume_controller = new VolumeController();
}

void
DeinitialiseVolumeController()
{
  assert(nullptr != volume_controller);

  delete volume_controller;
  volume_controller = nullptr;
}
