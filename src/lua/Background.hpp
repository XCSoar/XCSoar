// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Ptr.hpp"

namespace Lua {

/**
 * Add a Lua script that runs in background.
 *
 * It does not actually run, but has registered a callback that will
 * be invoked eventually.  See Lua::IsPersistent().
 */
void
AddBackground(StatePtr &&state) noexcept;

/**
 * Stop all background scripts.  Call this before shutting down
 * XCSoar.
 */
void
StopAllBackground() noexcept;

}
