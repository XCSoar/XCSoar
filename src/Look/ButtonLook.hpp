// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Brush.hpp"

class Font;

struct ButtonLook {
  const Font *font;

  struct StateLook {
    Color foreground_color;
    Brush foreground_brush;

    Color background_color;

    /**
     * Background while the button is pressed down (the CSS `active`
     * state).
     */
    Color pressed_background_color;

    /**
     * Caption and symbols while the button is pressed down; the
     * pressed face does not always carry the normal foreground.
     */
    Color pressed_foreground_color;
    Brush pressed_foreground_brush;

    /**
     * Hairline border drawn on the face outline, like a Tailwind
     * inset ring.
     */
    Color ring_color;
  } standard, selected, focused;

  /**
   * Solid ring hugging the focused button face from the outside,
   * like a Tailwind `ring-3` in the palette's light primary.
   */
  Color focus_ring_color;

  /**
   * The same outside ring for the selected button, in a darker
   * shade; the two share their face color.
   */
  Color selected_ring_color;

  struct {
    Color color;
    Brush brush;

    /**
     * Face of a disabled button; a step towards the page
     * background, so it stops looking like a raised card.
     */
    Color background_color;

    /**
     * Border of a disabled button: the page background color, which
     * trims the face and leaves no visible outline.
     */
    Color ring_color;
  } disabled;

  void Initialise(const Font &_font, bool dark_mode = false);
};
