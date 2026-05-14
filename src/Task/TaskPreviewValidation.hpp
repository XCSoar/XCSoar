// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Task/AbstractTask.hpp"

/**
 * True when the active task may use live map geometry (main map or inset
 * previews that mirror navigation state).  Draft task editor previews ignore
 * this gate and draw full geometry.
 */
[[gnu::pure]]
inline bool
IsLiveTaskPreviewValid(const AbstractTask *task) noexcept
{
  return task != nullptr && !IsError(task->CheckTask());
}
