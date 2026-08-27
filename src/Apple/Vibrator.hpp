// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

namespace Apple {

/**
 * Check whether this device is able to generate haptic feedback
 * (i.e. whether it has a "Taptic Engine").
 */
[[nodiscard]]
bool
HaveHapticFeedback() noexcept;

/**
 * Generate a short haptic feedback impulse.  This function has no
 * effect if the device is not able to generate haptic feedback.
 *
 * Note that iOS obeys the "System Haptics" setting internally, i.e. no
 * feedback is generated if the user has disabled it.
 */
void
VibrateShort() noexcept;

} // namespace Apple

#endif // TARGET_OS_IPHONE
#endif // __APPLE__
