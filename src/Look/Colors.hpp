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
 * Dark mode color palette derived from the XCSoar brand color.
 */
static constexpr Color COLOR_DARK_THEME_BACKGROUND =
  Color(0x0a, 0x15, 0x1f);
static constexpr Color COLOR_DARK_THEME_CAPTION =
  Color(0x10, 0x10, 0x10);
static constexpr Color COLOR_DARK_THEME_CAPTION_INACTIVE =
  Color(0x30, 0x30, 0x30);
static constexpr Color COLOR_DARK_THEME_LIST =
  Color(0x2a, 0x2a, 0x2a);
static constexpr Color COLOR_DARK_THEME_LIST_SELECTED =
  Color(0x3a, 0x3a, 0x3a);

static constexpr uint8_t ALPHA_OVERLAY = 0xA0;
