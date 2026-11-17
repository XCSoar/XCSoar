// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Point2D.hpp"

#include <span>
#include <vector>

class Color;
struct BulkPixelPoint;

/**
 * Lines with round ends and joins and a smooth, optionally wide and
 * soft edge, drawn with #OpenGL::round_line_shader.  Unlike a wide
 * polyline, the joins stay round even where a line turns back on
 * itself.  Each segment may have its own radius; a segment whose ends
 * are the same point is a dot.
 *
 * Collect the segments, then call Draw() for each colour; a soft
 * shadow is a second #RoundLines with a wide soft edge.
 */
class RoundLines {
  struct Vertex {
    FloatPoint2D position;

    /** the end points of the segment this vertex belongs to */
    FloatPoint2D a, b;

    float radius;
  };

  std::vector<Vertex> vertices;

  /** the stencil bit set by DrawMask() */
  static constexpr unsigned MASK_BIT = 2;

  /** the stencil bit of the pixels painted since BeginGroup() */
  static constexpr unsigned GROUP_BIT = 4;

  /** the width of the soft edge, centred on the radius */
  float softness;

public:
  /**
   * @param _softness the width of the soft edge in pixels; 1 is a
   * crisp, anti-aliased edge
   */
  explicit RoundLines(float _softness=1) noexcept
    :softness(_softness) {}

  bool empty() const noexcept {
    return vertices.empty();
  }

  void clear() noexcept {
    vertices.clear();
  }

  void AddSegment(FloatPoint2D a, FloatPoint2D b, float radius);

  void AddDot(FloatPoint2D center, float radius) {
    AddSegment(center, center, radius);
  }

  /**
   * Add a polyline.
   */
  void AddLine(std::span<const BulkPixelPoint> points,
               float radius);

  /**
   * Draw all segments.  A translucent colour is blended only once
   * where segments overlap, so their joints do not show.
   *
   * @param masked leave the pixels marked by the last DrawMask()
   * alone
   * @param grouped leave the pixels alone which another grouped
   * Draw() has painted since the last BeginGroup(), and mark the
   * ones painted now
   */
  void Draw(Color color, bool masked=false,
            bool grouped=false) const noexcept;

  /**
   * Start a group of Draw() calls with grouped=true.  Translucent
   * lines of several colours are then blended only once where they
   * cross or join each other; the first one drawn is on top.
   */
  static void BeginGroup() noexcept;

  /**
   * Mark the pixels these segments cover in the stencil buffer
   * without drawing them, to keep a following Draw() with
   * masked=true off them; for example the shadow below a translucent
   * line, which would otherwise show through.
   */
  void DrawMask() const noexcept;

private:
  /**
   * Select the shader and the vertices, draw, and deselect them.
   */
  template<typename F>
  void WithVertices(Color color, F &&draw) const noexcept;
};
