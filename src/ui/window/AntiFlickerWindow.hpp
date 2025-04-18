// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#if defined(ENABLE_OPENGL) || defined(USE_MEMORY_CANVAS)

#include "FakeBufferWindow.hpp"

/**
 * A #PaintWindow implementation that avoids flickering.  Some
 * platforms such as Windows draw directly to the screen, which may
 * expose the window before drawing has finished.  On these,
 * #AntiFlickerWindow will be buffered.  On OpenGL/SDL, which both have
 * full-screen double-buffering, this class is a simple #PaintWindow
 * without extra buffering.
 *
 * Note that this class is not supposed to reduce the number of
 * redraws when this is expensive.  Use it only when flicker avoidance
 * is the goal.
 */
class AntiFlickerWindow : public FakeBufferWindow {
};

#else

#include "BufferWindow.hpp"

class AntiFlickerWindow : public BufferWindow {
};

#endif
