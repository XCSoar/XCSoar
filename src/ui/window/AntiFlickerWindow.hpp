// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FakeBufferWindow.hpp"

/**
 * A #PaintWindow implementation that avoids flickering.  OpenGL, SDL,
 * and the memory canvas already double-buffer the full screen, so
 * this class is a simple #PaintWindow without extra buffering.
 *
 * Note that this class is not supposed to reduce the number of
 * redraws when this is expensive.  Use it only when flicker avoidance
 * is the goal.
 */
class AntiFlickerWindow : public FakeBufferWindow {
};
