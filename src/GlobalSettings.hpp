// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

/**
 * This namespace provides access to configuration settings in the
 * operating system.  This allows XCSoar to inherit global settings.
 */
namespace GlobalSettings {

#if defined(ANDROID) || defined(__APPLE__)
inline bool dark_mode = false;
#else
static constexpr bool dark_mode = false;
#endif

#ifdef ANDROID
inline bool haptic_feedback = false;
#endif

#ifdef __APPLE__
#if TARGET_OS_IPHONE
/* iOS has no API for querying the "System Haptics" setting, but
   UIFeedbackGenerator obeys it internally; therefore "OS settings"
   simply means "enabled" here */
static constexpr bool haptic_feedback = true;
#endif
#endif

} // namespace GlobalSettings
