// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include <atomic>

// Variable to track if cursor keys are available on iOS
extern std::atomic<bool> has_cursor_keys;

#ifdef ENABLE_SDL
// Function to detect cursor keys and set the flag
void DetectCursorKeysiOS(unsigned key_code) noexcept;
#endif // ENABLE_SDL

#endif // TARGET_OS_IPHONE
#endif // __APPLE__
