// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BufferWindow.hpp"

/**
 * A #PaintWindow implementation which avoids calling OnPaint() unless
 * Invalidate() has been called explicitly.  Implementations which
 * redraw the whole screen at a time (OpenGL, memory canvas) need a
 * buffer.
 */
class LazyPaintWindow : public BufferWindow {
};
