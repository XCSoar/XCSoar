// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_WINUSER

#include "Window.hpp"
using NativeWindow = Window;

#else

#include "PaintWindow.hpp"

/**
 * A base class for Window implementations that use a "native" GDI
 * window class.  On non-GDI, this is an alias for PaintWindow, and
 * everything needs to be reimplemented.
 */
using NativeWindow = PaintWindow;

#endif
