// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

#ifdef ENABLE_SDL
#include <SDL_platform.h>
#include <SDL_opengles2.h>
#else
#include <GLES2/gl2.h>
#endif
