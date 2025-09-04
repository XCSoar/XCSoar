// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

/**
 * Returns true when an external hardware keyboard is connected.
 *
 * @return True if an external hardware keyboard is connected.
 */
[[nodiscard]]
bool IsHardwareKeyboardConnected() noexcept;

#endif // TARGET_OS_IPHONE
#endif // __APPLE__
