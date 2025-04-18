// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VertexPointer.hpp"
#include "ui/dim/BulkPoint.hpp"

template<unsigned n>
struct GLVertexArray {
  static constexpr unsigned SIZE = n;

  BulkPixelPoint v[SIZE];

  void Bind(ScopeVertexPointer &vp) const noexcept {
    vp.Update(v);
  }
};

struct GLDonutVertices : public GLVertexArray<66> {
  static constexpr unsigned CIRCLE_SIZE = (SIZE - 2) / 2;
  static constexpr unsigned MAX_ANGLE = CIRCLE_SIZE * 2u;

  static constexpr unsigned FixAngle(unsigned angle) noexcept {
    return angle & ((CIRCLE_SIZE - 1u) * 2u);
  }

  static constexpr unsigned PreviousAngle(unsigned angle) noexcept {
    return FixAngle(angle - 2u);
  }

  static constexpr unsigned NextAngle(unsigned angle) noexcept {
    return FixAngle(angle + 2u);
  }

  static constexpr unsigned ImportAngle(unsigned other_angle,
                                        unsigned other_max) noexcept {
    return ((other_angle * CIRCLE_SIZE / other_max) % CIRCLE_SIZE) * 2u;
  }

  GLDonutVertices(GLvalue center_x, GLvalue center_y,
                  GLvalue radius_inner, GLvalue radius_outer) noexcept;

  void BindInnerCircle(ScopeVertexPointer &vp) const noexcept {
    vp.Update(GL_VALUE, sizeof(v[0]) * 2, v);
  }

  void BindOuterCircle(ScopeVertexPointer &vp) const noexcept {
    vp.Update(GL_VALUE, sizeof(v[0]) * 2, v + 1);
  }
};
