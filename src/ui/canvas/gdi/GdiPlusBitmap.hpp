// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#if defined(_WIN32) && defined(USE_GDI)

#include <tchar.h>
#include <windef.h>

HBITMAP GdiLoadImage(const TCHAR* filename);
void GdiStartup();
void GdiShutdown();
#endif  // _WIN32 && USE_GDI
