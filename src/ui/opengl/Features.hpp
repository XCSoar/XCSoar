// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#ifndef ENABLE_OPENGL
#error No OpenGL
#endif

#define HAVE_TEXT_CACHE

#if defined(_WIN32) || defined(MESA_KMS) || defined(USE_X11) || defined(ENABLE_SDL) || defined(USE_WAYLAND)
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
#define HAVE_DYNAMIC_MAPBUFFER
#endif // !iPhone
#endif

#if defined(MESA_KMS)
#define DRAW_MOUSE_CURSOR
#endif

#if !defined(ANDROID) && !defined(TARGET_OS_IPHONE)
/**
 * Support display rotation via glRotatef()?
 */
#define SOFTWARE_ROTATE_DISPLAY
#endif
