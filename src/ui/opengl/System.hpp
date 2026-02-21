// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

#ifdef ENABLE_SDL
#include <SDL_platform.h>

#ifdef HAVE_GLES2
#include <SDL_opengles2.h>
#endif

#elif defined(HAVE_GLES2)
#include <GLES2/gl2.h>
#endif

// Windows Desktop OpenGL (no ANGLE or other GLES)
// Use glad for GL function loading
#if defined(_WIN32) && !defined(HAVE_GLES2)
#include <glad/glad.h>
#endif
