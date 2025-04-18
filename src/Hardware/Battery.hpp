// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PowerFeatures.hpp"

#if defined(HAVE_BATTERY) && !defined(ANDROID)

namespace Power {

/* note: this function is not implemented on Android, because a JNI
   callback will update the global variable there */

[[gnu::pure]]
struct Info
GetInfo() noexcept;

} // namespace Power

#endif /* HAVE_BATTERY */
