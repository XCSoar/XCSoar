// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"

#ifdef XCSOAR_TESTING
static constexpr Color COLOR_XCSOAR_LIGHT = Color(0xed, 0x90, 0x90);
static constexpr Color COLOR_XCSOAR = Color(0xd0, 0x17, 0x17);
static constexpr Color COLOR_XCSOAR_DARK = Color(0x5d, 0x0a, 0x0a);
#else
static constexpr Color COLOR_XCSOAR_LIGHT = Color(0xaa, 0xc9, 0xe4);
static constexpr Color COLOR_XCSOAR = Color(0x3f, 0x76, 0xa8);
static constexpr Color COLOR_XCSOAR_DARK = Color(0x00, 0x31, 0x5e);
#endif

/**
 * Dark mode color palette: a cool neutral, so little of the brand
 * blue left that it reads as a gray rather than as a color of its
 * own.  The card and the button are one lighter step of the page,
 * the same step a white face takes on the sand page.  A selected
 * row is one step lighter than the card.
 */
static constexpr Color COLOR_DARK_THEME_BACKGROUND =
  Color(0x15, 0x17, 0x1a);
static constexpr Color COLOR_DARK_THEME_CAPTION =
  Color(0x08, 0x09, 0x0b);
static constexpr Color COLOR_DARK_THEME_CAPTION_INACTIVE =
  Color(0x28, 0x2b, 0x2d);
static constexpr Color COLOR_DARK_THEME_LIST =
  Color(0x3a, 0x3e, 0x44);
static constexpr Color COLOR_DARK_THEME_LIST_SELECTED =
  Color(0x4c, 0x51, 0x58);
static constexpr Color COLOR_DARK_THEME_BUTTON =
  COLOR_DARK_THEME_LIST;

/** A pressed row: one step lighter than the card. */
static constexpr Color COLOR_DARK_THEME_LIST_PRESSED =
  MixColors(COLOR_WHITE, COLOR_DARK_THEME_LIST, 0x28);

/**
 * Light mode dialog background color: a sand so far desaturated that
 * it keeps the warmth of paper without turning the page yellow.
 * Dark enough that a white button is a lighter step of the page, the
 * same direction as the dark-mode face.
 */
static constexpr Color COLOR_DIALOG_BACKGROUND =
  Color(0xc9, 0xc4, 0xbc);

/**
 * Flat "card" button face (light mode): a white card, the lighter
 * step of the sand page.  The outline is the hairline on that white;
 * a warm border on a warm page reads as dirt.
 */
static constexpr Color COLOR_BUTTON_FACE =
  Color(0xff, 0xff, 0xff);
static constexpr Color COLOR_BUTTON_RING =
  Color(0xa8, 0xa7, 0xa5);
static constexpr Color COLOR_BUTTON_PRESSED =
  Color(0xd0, 0xcc, 0xc2);

/**
 * A disabled button stays between the page and the white cards,
 * so it no longer reads as a raised, clickable card.
 */
static constexpr Color COLOR_BUTTON_DISABLED =
  Color(0xf4, 0xf1, 0xec);
static constexpr Color COLOR_BUTTON_DISABLED_TEXT =
  Color(0x93, 0x90, 0x8b);

/**
 * The primary color further down its own scale, like going from a
 * Tailwind `primary-600` to `primary-800`.
 */
static constexpr Color COLOR_XCSOAR_PRESSED =
  MixColors(COLOR_XCSOAR, COLOR_XCSOAR_DARK, 0x4d);

/**
 * Grouped-list badge fills.  The label on each is white.  The accent
 * badge uses the dialog focus colours instead of a constant here.
 * Yellow-500, red-600 and green-700.
 */
static constexpr Color COLOR_BADGE_WARNING = Color(0xea, 0xb3, 0x08);
static constexpr Color COLOR_BADGE_DANGER = Color(0xdc, 0x26, 0x26);
static constexpr Color COLOR_BADGE_SUCCESS = Color(0x15, 0x80, 0x3d);

/**
 * Admonition colors for Markdown rendering.
 */
static constexpr Color COLOR_ADMONITION_IMPORTANT =
  Color(0xd0, 0x6b, 0x00);
static constexpr Color COLOR_ADMONITION_IMPORTANT_DARK =
  Color(0xff, 0xa0, 0x30);
static constexpr Color COLOR_ADMONITION_TIP =
  Color(0x00, 0x80, 0x00);

/**
 * A muted green readable on light backgrounds.
 * Standard COLOR_GREEN (0,255,0) is too bright on white.
 */
static constexpr Color COLOR_LIGHT_GREEN = Color(0x00, 0xc0, 0x00);

/**
 * A dark amber readable on light backgrounds.
 * Standard COLOR_YELLOW (255,255,0) is invisible on white.
 */
static constexpr Color COLOR_AMBER = Color(0xb3, 0x5a, 0x00);

/**
 * Airspace warning list / map-item status badge colours.
 */
static constexpr Color COLOR_AIRSPACE_WARNING_INSIDE =
  Color(254, 50, 50);
static constexpr Color COLOR_AIRSPACE_WARNING_NEAR =
  Color(254, 254, 50);
static constexpr Color COLOR_AIRSPACE_WARNING_INSIDE_ACK =
  Color(254, 100, 100);
static constexpr Color COLOR_AIRSPACE_WARNING_NEAR_ACK =
  Color(254, 254, 100);

/**
 * XCTherm overlay palette.
 */
static constexpr Color COLOR_XCTHERM_BLUE = Color(0x00, 0x00, 0xc8);
static constexpr Color COLOR_XCTHERM_BRIGHT_CYAN = Color(0x00, 0xd0, 0xff);
static constexpr Color COLOR_XCTHERM_SKY_BLUE = Color(0x60, 0xe0, 0xff);
static constexpr Color COLOR_XCTHERM_LIGHT_BLUE = Color(0xa0, 0xe8, 0xff);
static constexpr Color COLOR_XCTHERM_PALE_BLUE = Color(0xc8, 0xf0, 0xff);
static constexpr Color COLOR_XCTHERM_CREAM = Color(0xf0, 0xf0, 0xc0);
static constexpr Color COLOR_XCTHERM_YELLOW = Color(0xff, 0xff, 0x00);
static constexpr Color COLOR_XCTHERM_GOLD = Color(0xff, 0xd0, 0x00);
static constexpr Color COLOR_XCTHERM_ORANGE = Color(0xff, 0xa0, 0x00);
static constexpr Color COLOR_XCTHERM_RED_ORANGE = Color(0xff, 0x40, 0x00);
static constexpr Color COLOR_XCTHERM_RED = Color(0xff, 0x00, 0x00);
static constexpr Color COLOR_XCTHERM_PURPLE = Color(0xa0, 0x20, 0xf0);

static constexpr uint8_t ALPHA_OVERLAY = 0xA0;
