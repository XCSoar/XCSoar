// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace Android {

/**
 * How far the top of the XCSoar view still lies inside Android's
 * system swipe-down band, in pixels.  Zero when that band does not
 * cover the view.
 */
[[nodiscard]]
int
GetTopGestureClearance() noexcept;

} // namespace Android
