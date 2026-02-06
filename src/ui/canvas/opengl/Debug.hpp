// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifndef NDEBUG

#ifdef _WIN32
#include <windows.h>

namespace OpenGL {
extern DWORD thread;
} // namespace OpenGL

#else
#include <pthread.h>

namespace OpenGL {
extern pthread_t thread;
} // namespace OpenGL

#endif
#endif
