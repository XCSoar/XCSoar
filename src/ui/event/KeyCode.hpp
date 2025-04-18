// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ANDROID
#include "android/KeyCode.hpp"
#elif defined(USE_POLL_EVENT)
#include "poll/KeyCode.hpp"
#elif defined(ENABLE_SDL)
#include "sdl/KeyCode.hpp"
#else
#include "windows/KeyCode.hpp"
#endif
