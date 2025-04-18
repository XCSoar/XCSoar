// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_GDI

#include "FakeBufferWindow.hpp"

class LazyPaintWindow : public FakeBufferWindow {
};

#else

#include "BufferWindow.hpp"

/**
 * A #PaintWindow implementation which avoids calling OnPaint() unless
 * Invalidate() has been called explicitly.  It will try to avoid
 * OnPaint() if the old screen contents are still available (which is
 * only possible with GDI).  Implementations which require XCSoar to
 * redraw the whole screen at a time (like OpenGL) need a buffer.
 */
class LazyPaintWindow : public BufferWindow {
};

#endif
