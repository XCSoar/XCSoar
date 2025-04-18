// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_WAYLAND
#include <wayland-egl.h>
#endif

#ifdef USE_X11
/* kludges to work around namespace collisions with X11 headers */
#define Font X11Font
#define Window X11Window
#define Display X11Display
#endif

#ifdef MESA_KMS
#include <gbm.h>
#endif

#include <EGL/egl.h>

#ifdef USE_X11
#undef Font
#undef Window
#undef Display

#ifdef NoValue
#undef NoValue
#endif

#ifdef None
#undef None
#endif

#ifdef Below
#undef Below
#endif

#endif
