// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

static constexpr unsigned NUMDEV = 6;
static constexpr unsigned INTERNAL_DEVICE_SLOT = NUMDEV - 1;

#if defined(ANDROID) || defined(__APPLE__)
#define HAVE_INTERNAL_GPS
#endif
