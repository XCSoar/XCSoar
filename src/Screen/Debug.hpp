// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef NDEBUG

static inline void
ScreenInitialized() {}

static inline void
ScreenDeinitialized() {}

#else

/**
 * Call this when the screen library has been initialized.
 */
void
ScreenInitialized();

/**
 * Call this when the screen library has been deinitialized.
 */
void
ScreenDeinitialized();

/**
 * Determine if the screen library has been initialized and is
 * available.
 */
bool
IsScreenInitialized();

#endif
