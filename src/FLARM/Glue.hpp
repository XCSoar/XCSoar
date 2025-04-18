// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Load all FLARM databases into memory, suspending the MergeThread.
 * This is a no-op if this has been attempted already.
 */
void
LoadFlarmDatabases() noexcept;

/**
 * Same as LoadFlarmDatabases except that this method forces reload even if the database
 * has been loaded previously.
 */
void
ReloadFlarmDatabases() noexcept;

void
SaveFlarmColors() noexcept;

void
SaveFlarmNames() noexcept;

void
DeinitTrafficGlobals() noexcept;
