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
SaveFlarmMessaging() noexcept;

/**
 * Save the FLARM messaging database at most every few minutes.
 * Intended for frequent update call sites (e.g., parser) to avoid
 * excessive disk writes while still persisting data regularly.
 */
void
SaveFlarmMessagingPeriodic() noexcept;

void
DeinitTrafficGlobals() noexcept;
