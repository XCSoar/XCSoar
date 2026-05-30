// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Font.hpp"

class Font;

struct VarioLook {
  bool inverse, colors;

  Color background_color, text_color, dimmed_text_color;

  Color sink_color, lift_color;

  /** Infobox palette — same indices as #InfoBoxLook::colors. */
  Color accent[6];

  Color scale_face_color;
  Color scale_ink_color;

  Pen arc_pen, tick_pen;
  Font arc_label_font;
  Font scale_unit_font;
  Font hint_font;

  Brush background_brush;
  Brush sink_brush, lift_brush;

  Pen thick_background_pen, thick_sink_pen, thick_lift_pen;

  Bitmap climb_bitmap;

  const Font *text_font;
  Font value_font;

  Font unit_font;
  Pen unit_fraction_pen;

  Font label_font;

  /** Incremented by ReinitialiseLayout(); gauge uses this to detect font changes. */
  unsigned layout_generation = 0;

  /** Inner text area width as % of inscribed square (trimmed from the right). */
  static constexpr unsigned INNER_TEXT_WIDTH_PERCENT = 85;

  /** Label column as % of inner text area width (remainder is values). */
  static constexpr unsigned INNER_ROW_LABEL_WIDTH_PERCENT = 38;

  /** Extra width budget when fitting inner row fonts (110 = 10% larger). */
  static constexpr unsigned INNER_ROW_FONT_PERCENT = 110;

  /** Extra width budget for inner value column (MC, Gross, Net.). */
  static constexpr unsigned INNER_VALUE_FONT_PERCENT = 130;

  /** Label font height as % of value font height. */
  static constexpr unsigned INNER_LABEL_FONT_PERCENT = 72;

  /** Scale unit label height as % of #arc_label_font height. */
  static constexpr unsigned SCALE_UNIT_FONT_PERCENT = 78;

  /** MC mode hint max height as % of #label_font when space allows. */
  static constexpr unsigned HINT_FONT_PERCENT = 100;

  void Initialise(bool inverse, bool colors,
                  unsigned width,
                  const Font &text_font);

  void ReinitialiseLayout(unsigned width);

  void ReinitialiseLayout(unsigned panel_width, unsigned scale_title_font,
                          unsigned text_column_width,
                          unsigned max_row_height,
                          unsigned arc_label_width) noexcept;

  /** Size #hint_font to fit the MC mode strip; does not bump #layout_generation. */
  void FitHintFont(unsigned max_height, unsigned max_width) noexcept;
};
