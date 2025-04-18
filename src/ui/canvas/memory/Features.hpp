// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#define HAVE_TEXT_CACHE

#define HAVE_ALPHA_BLEND

#if defined(USE_FB) && !defined(KOBO)
#define DRAW_MOUSE_CURSOR
#endif
