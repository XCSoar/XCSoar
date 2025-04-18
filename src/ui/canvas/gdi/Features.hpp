// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * This macro is defined when the Canvas implements clipping against
 * its siblings and children.
 */
#define HAVE_CLIPPING

#define HAVE_HATCHED_BRUSH

/* AlphaBlend() is implemented since Windows 2000 */
#if _WIN32_WINDOWS >= 0x500
#define HAVE_ALPHA_BLEND
#endif
