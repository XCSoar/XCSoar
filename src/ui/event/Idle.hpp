// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Check whether the user is currently inactive.
 *
 * When the user is currently interacting with XCSoar, we should
 * attempt to reduce UI latency, for example by reducing rendering
 * details.
 *
 * @return true if the user has been idle for at the specified number
 * of milliseconds or more
 */
[[gnu::pure]]
bool
IsUserIdle(unsigned duration_ms) noexcept;

/**
 * Acts as if the user had just interacted with XCSoar.
 */
void
ResetUserIdle() noexcept;
