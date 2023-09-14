// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/* kludges to work around namespace collisions with X11 headers */
#define Font X11Font
#define Window X11Window
#define Display X11Display

#include <GL/glx.h>

#undef Font
#undef Window
#undef Display

#ifdef Expose
#undef Expose
#endif

#ifdef NoValue
#undef NoValue
#endif

#ifdef None
#undef None
#endif

#ifdef Below
#undef Below
#endif
