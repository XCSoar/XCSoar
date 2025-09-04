// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/**
 * @file KeyboardDetection.cpp
 * @brief iOS keyboard detection functionality for XCSoar
 * 
 * This module provides functionality to detect external keyboard availability
 * on iOS devices. It monitors for cursor key input events to determine when
 * an external keyboard with arrow keys is connected, which allows the application
 * to adapt its user interface and input handling accordingly.
 * 
 * This iOS implementation parallels the Android version in EventBridge.cpp
 * and integrates with the platform detection system in Asset.hpp, specifically
 * the HasCursorKeys() function which uses the has_cursor_keys flag to determine
 * UI behavior across different platforms.
 * 
 */

#include "KeyboardDetection.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#ifdef ENABLE_SDL
#include "ui/event/sdl/KeyCode.hpp"
#endif

#include <atomic>

/**
 * @brief Flag indicating whether cursor keys (arrow keys) are available.
 * 
 * This variable tracks whether the iOS device has cursor keys available,
 * which typically occurs when an external keyboard (or e.g. a XCRemote) is connected.
 * It starts as false and is set to true when any cursor key is detected.
 * 
 * Uses std::atomic to ensure thread-safe access between input event processing
 * and UI rendering threads.
 */
std::atomic<bool> has_cursor_keys{false};

/**
 * @brief Detects cursor key input on iOS devices to determine keyboard availability.
 * 
 * This function monitors key input events to detect when cursor keys (arrow keys)
 * are pressed on iOS devices. When a cursor key is detected, it sets the global
 * has_cursor_keys flag to true, indicating that an external keyboard with cursor
 * keys is available.
 * 
 * @param key_code The key code of the pressed key to check against cursor keys
 * 
 * @note This function is only available when SDL is enabled (ENABLE_SDL is defined)
 *       because it depends on SDL key codes (KEY_UP, KEY_DOWN, etc.) from
 *       "ui/event/sdl/KeyCode.hpp". On iOS, SDL is used for the event handling.
 * @note The detection is permanent - once cursor keys are detected, the flag
 *       remains true for the duration of the application session
 * @note Function is marked noexcept as it only performs atomic operations
 *       and basic comparisons that cannot throw exceptions
 * @see Android implementation in EventBridge.cpp for similar functionality
 * @see Asset.hpp HasCursorKeys() which uses this flag on iOS and Android
 * @see InputEvents.cpp ProcessKey() which processes the detected key events
 */
#ifdef ENABLE_SDL
void
DetectCursorKeysiOS(unsigned key_code) noexcept
{
  // Check if this is a cursor key (UP, DOWN, LEFT, RIGHT)
  // Use relaxed memory ordering since this is a simple flag that only transitions false->true
  if (!has_cursor_keys.load(std::memory_order_relaxed) && 
      (key_code == KEY_UP ||
       key_code == KEY_DOWN ||
       key_code == KEY_LEFT ||
       key_code == KEY_RIGHT)) {
      has_cursor_keys.store(true, std::memory_order_relaxed);
  }
}
#endif // ENABLE_SDL

#endif // TARGET_OS_IPHONE
#endif // __APPLE__
