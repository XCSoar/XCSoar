// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * This namespace provides access to configuration settings in the
 * operating system.  This allows XCSoar to inherit global settings.
 */
namespace GlobalSettings {

#ifdef ANDROID
inline bool dark_mode = false;
inline bool haptic_feedback = false;
#else
static constexpr bool dark_mode = false;
#endif

} // namespace GlobalSettings
