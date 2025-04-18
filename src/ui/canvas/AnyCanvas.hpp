// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ENABLE_OPENGL

#include "ui/canvas/Canvas.hpp"

class AnyCanvas : public Canvas {};

#else

#include "ui/canvas/VirtualCanvas.hpp"

class AnyCanvas : public VirtualCanvas {
public:
  AnyCanvas():VirtualCanvas({1, 1}) {}
};

#endif
