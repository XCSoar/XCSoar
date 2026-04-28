// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#define HAVE_MULTI_TOUCH

#if defined(ENABLE_OPENGL) && (defined(__APPLE__) || defined(_WIN32))
#define HAVE_HIGHDPI_SUPPORT
#endif
