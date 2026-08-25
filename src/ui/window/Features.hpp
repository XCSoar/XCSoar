// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ANDROID
#include "android/Features.hpp"
#endif

#ifdef ENABLE_SDL
#include "sdl/Features.hpp"
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

/**
 * Can this platform hide the system bars (status bar, navigation bar,
 * home indicator) at runtime, and let XCSoar use the whole screen?
 *
 * @see DisplaySettings::full_screen
 */
#if defined(ANDROID) || (defined(__APPLE__) && TARGET_OS_IPHONE)
#define HAVE_FULL_SCREEN_SETTING
#endif

/**
 * Can the status bar be shown and hidden independently of the full
 * screen setting?
 *
 * @see DisplaySettings::status_bar
 */
#if defined(__APPLE__) && TARGET_OS_IPHONE
#define HAVE_STATUS_BAR_SETTING
#endif
