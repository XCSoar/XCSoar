// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#if defined(_WIN32) && defined(USE_GDI)

#include <windef.h>
#include <string_view>

HBITMAP GdiLoadImage(std::string_view filename);
void GdiStartup();
void GdiShutdown();
#endif  // _WIN32 && USE_GDI
