// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>
#include <cstdint>

struct PixelRect;

/**
 * A soft black shadow, like a CSS "box-shadow" without an offset.  It
 * consists of up to two layers, which are drawn on top of each other:
 * a wide, soft one for a gentle fade, and a tight one which darkens
 * the area close to the box.
 */
struct BoxShadowStyle {
  /**
   * One layer: the box's shape, moved out by #spread and blurred.
   * Sizes are in virtual points (see Layout::VptScale()).
   */
  struct Layer {
    /**
     * How far the shape reaches beyond the box; a negative value
     * starts it inside the box, so the shadow is light at the box's
     * edge.
     */
    int spread;

    /**
     * The width of the blurred transition, centred on the edge of
     * the shape.  A CSS blur radius corresponds to half of this.
     */
    unsigned blur;

    /** The opacity where the layer is darkest; 0 for no layer */
    uint8_t alpha;

    [[gnu::pure]]
    int GetScaledSpread() const noexcept;

    [[gnu::pure]]
    unsigned GetScaledBlur() const noexcept;
  };

  std::array<Layer, 2> layers;

  /**
   * The shadow of a dialog: dark, and reaching far beyond it, which
   * sets it clearly apart from everything behind it.
   */
  static const BoxShadowStyle DIALOG;

  /**
   * A light shadow for small elements which float above the map,
   * like labels and the gesture trail: it starts inside the element,
   * so the map right next to it is hardly darkened.
   */
  static const BoxShadowStyle FLOATING;
};

inline constexpr BoxShadowStyle BoxShadowStyle::DIALOG{{{
  {6, 32, 115},
}}};

inline constexpr BoxShadowStyle BoxShadowStyle::FLOATING{{{
  {-3, 30, 0x35},
  {-4, 14, 0x35},
}}};

/**
 * Draw a soft black drop shadow around the given rectangle, to make it
 * look like it floats above what is painted behind it.  The shadow
 * surrounds the rectangle evenly on all four sides, and its corners
 * are rounded, the way the corners of a blurred rectangle are.
 *
 * This must be called before the box itself is painted: the shadow is
 * drawn as a solid shape which is blurred at its edges, so the area
 * covered by the box gets painted over as well.
 *
 * The rectangle is relative to the current Canvas, and the shadow
 * extends beyond it, which means the caller must be allowed to paint
 * outside of its own window.
 *
 * This is implemented with OpenGL and does nothing on other platforms.
 *
 * @param corner_radius the radius of the box's rounded corners
 */
void
DrawBoxShadow(const PixelRect &rc, const BoxShadowStyle &style,
              unsigned corner_radius=0) noexcept;
