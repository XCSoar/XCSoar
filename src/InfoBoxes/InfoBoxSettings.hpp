// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"
#include "util/Compiler.h"
#include "InfoBoxes/Content/Type.hpp"

#include <cstdint>

struct InfoBoxSettings {
  enum PanelIndex {
    PANEL_CIRCLING,
    PANEL_CRUISE,
    PANEL_FINAL_GLIDE,
    PANEL_AUXILIARY,
  };

  struct Panel {
    static constexpr unsigned MAX_CONTENTS = 24;

    StaticString<32u> name;
    InfoBoxFactory::Type contents[MAX_CONTENTS];

    void Clear() noexcept;

    [[gnu::pure]]
    bool IsEmpty() const noexcept;
  };

  static constexpr unsigned MAX_PANELS = 8;
  static constexpr unsigned PREASSIGNED_PANELS = 3;

  /**
   * Auto-switch to the "final glide" panel if above final glide?
   * This setting affects the #DisplayMode, and is checked by
   * GetNewDisplayMode().
   */
  bool use_final_glide;

  enum class Geometry : uint8_t {
    /** 8 infoboxes split bottom/top or left/right */
    SPLIT_8,

    /** 8 infoboxes along bottom or right */
    BOTTOM_RIGHT_8 = 1,

    /** 8 infoboxes along top or left */
    TOP_LEFT_8 = 2,

    /** @see #SPLIT_8 */
    OBSOLETE_SPLIT_8 = 3,

    /** @see #TOP_LEFT_8 */
    OBSOLETE_TOP_LEFT_8 = 4,

    /** @see #BOTTOM_RIGHT_8 */
    OBSOLETE_BOTTOM_RIGHT_8 = 5,

    /** 9 right + vario */
    RIGHT_9_VARIO = 6,

    /** infoboxes (5) along right side (square screen) */
    RIGHT_5 = 7,

    /** 12 infoboxes along bottom or right side */
    BOTTOM_RIGHT_12 = 8,

    /** 24 infoboxes along right side (3x8) */
    RIGHT_24 = 9,

    /** @see BOTTOM_RIGHT_12 */
    OBSOLETE_BOTTOM_RIGHT_12 = 10,

    /** 12 infoboxes along top or left */
    TOP_LEFT_12 = 11,

    /** 6 left, 3 right + vario */
    LEFT_6_RIGHT_3_VARIO = 12,

    /** 8 bottom + vario */
    BOTTOM_8_VARIO = 13,
    TOP_LEFT_4 = 14,
    BOTTOM_RIGHT_4 = 15,

    OBSOLETE_BOTTOM_RIGHT_4 = 16,
    OBSOLETE_TOP_LEFT_4 = 17,

    /** 8 top + vario */
    TOP_8_VARIO = 18,

    /** 16 infoboxes along right side (2x8) */
    RIGHT_16 = 19,
    LEFT_12_RIGHT_3_VARIO = 20,

    /** 10 infoboxes along top or left */
    TOP_LEFT_10 = 21,
    /** 10 infoboxes along bottom or right side */
    BOTTOM_RIGHT_10 = 22,
    /** 10 infoboxes split bottom/top or left/right */
    SPLIT_10 = 23,
    /** 12 infoboxes 3X4 split bottom/top or left/right */
    SPLIT_3X4 = 24,
    /** 15 infoboxes 3X5 split bottom/top or left/right */
    SPLIT_3X5 = 25,
    /** 18 infoboxes 3X6 split bottom/top or left/right */
    SPLIT_3X6 = 26,

  } geometry;

/*
 * scales the font for InfoBox titles and comments between 50% and 150%
 * the value of scale_title_font ranges from 50 to 150 accordingly.
 */
  unsigned scale_title_font;

  bool use_colors;

  enum class BorderStyle : uint8_t {
    BOX,
    TAB,
    SHADED,
    GLASS,
  } border_style;

  Panel panels[MAX_PANELS];

  void SetDefaults() noexcept;
};
