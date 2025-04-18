// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <shared_mutex>

#ifdef _WIN32

#include "WindowsSharedMutex.hxx"
using SharedMutex = WindowsSharedMutex;

#else

using SharedMutex = std::shared_mutex;

#endif
