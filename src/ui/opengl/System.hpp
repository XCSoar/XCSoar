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

#ifdef DRAW_REDRAW_COUNTER
namespace OpenGL {
extern unsigned frame_draw_calls;

inline void NoteDrawCall() noexcept {
  ++frame_draw_calls;
}
}

/* Parentheses stop the macro from calling itself. */
#define glDrawArrays(...) \
  (OpenGL::NoteDrawCall(), (glDrawArrays)(__VA_ARGS__))
#define glDrawElements(...) \
  (OpenGL::NoteDrawCall(), (glDrawElements)(__VA_ARGS__))
#endif
