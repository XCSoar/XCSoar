// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/opengl/Types.hpp"
#include "Math/Point2D.hpp"

struct ExactPixelPoint : Point2D<GLexact> {
  ExactPixelPoint() = default;

  constexpr ExactPixelPoint(GLexact _x, GLexact _y)
    :Point2D<GLexact>(_x, _y) {}

  constexpr ExactPixelPoint(PixelPoint p)
    :Point2D<GLexact>(ToGLexact(p.x), ToGLexact(p.y)) {}
};
