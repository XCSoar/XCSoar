// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Font.hpp"
#include "util/Macros.hpp"

class Font;

struct InfoBoxLook {
  unsigned border_width;

  bool inverse;

  Pen border_pen;
  Color background_color, focused_background_color, pressed_background_color;

  /**
   * The simplified InfoBox cards shown while the InfoBoxes are being
   * arranged (see #InfoBoxArrange).  The backdrop is the dialog
   * background.  #preview_active_color fills the card which follows
   * the finger, and #preview_focus_width is the filled selection
   * halo outside the hairline.  #preview_border_color is the
   * InfoBox separator gray in inverse, black in light.
   */
  Color preview_active_color, preview_border_color;
  unsigned preview_padding, preview_radius, preview_border_width,
    preview_focus_width;

  /**
   * Used only by #InfoBoxSettings::BorderStyle::SHADED.
   */
  Color caption_background_color;

  struct {
    Color fg_color;
  } title, value, comment;

  Font value_font, small_value_font;

  /**
   * The font for units.
   */
  Font unit_font;

  Pen unit_fraction_pen;

  Font title_font;
  Font title_font_bold;

  /** the small font for the slot number in the arrange preview */
  Font preview_number_font;

  Color colors[6];

  void Initialise(bool inverse, bool use_colors,
                  unsigned width, unsigned scale_title_font);

  void ReinitialiseLayout(unsigned width, unsigned scale_title_font);

  /** Colour of the long-press fill on an InfoBox or arrange card. */
  [[gnu::pure]]
  Color GetPreviewGlowColor() const noexcept;

  Color GetColor(int i, Color default_color) const {
    if (i < 0)
      return colors[0];
    else if (i >= 1 && (unsigned)i < ARRAY_SIZE(colors))
      return colors[i];
    else
      return default_color;
  }

  Color GetTitleColor(int i) const {
    return GetColor(i, title.fg_color);
  }

  Color GetValueColor(int i) const {
    return GetColor(i, value.fg_color);
  }

  Color GetCommentColor(int i) const {
    return GetColor(i, comment.fg_color);
  }
};
