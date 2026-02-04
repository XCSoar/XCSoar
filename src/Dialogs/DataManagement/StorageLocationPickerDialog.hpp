// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

/**
 * Show a modal dialog for selecting a writable storage location
 * (USB stick, SD card, etc.).  Returns the chosen mount path,
 * or a null path on cancel.  Automatically updates the
 * last-used target on success.
 */
AllocatedPath
PickStorageLocation() noexcept;

/**
 * Return the last storage location chosen by the user, or a null
 * path if none has been chosen yet.
 */
const AllocatedPath &
GetLastStorageTarget() noexcept;

/**
 * Override the last-used storage target (e.g. after a successful
 * operation that confirms the target is valid).
 */
void
SetLastStorageTarget(const AllocatedPath &p) noexcept;

/**
 * Convenience: show the picker and apply the result via a callback.
 * Does nothing if the user cancels.
 */
template<typename F>
void
PickStorageLocationAndApply(F &&fn) noexcept
{
  AllocatedPath chosen = PickStorageLocation();
  if (chosen != nullptr)
    fn(std::move(chosen));
}
