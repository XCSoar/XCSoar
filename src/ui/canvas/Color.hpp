// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

// IWYU pragma: begin_exports
#ifdef ENABLE_OPENGL
#include "opengl/Color.hpp"
#elif defined(USE_MEMORY_CANVAS)
#include "memory/Color.hpp"
#elif defined(USE_GDI)
#include "gdi/Color.hpp"
#else
#error No Color implementation
#endif
// IWYU pragma: end_exports

static constexpr Color COLOR_WHITE = Color(0xff, 0xff, 0xff);
static constexpr Color COLOR_BLACK = Color(0x00, 0x00, 0x00);
static constexpr Color COLOR_GRAY = Color(0x80, 0x80, 0x80);
static constexpr Color COLOR_VERY_LIGHT_GRAY = Color(0xd8, 0xd8, 0xd8);
static constexpr Color COLOR_LIGHT_GRAY = Color(0xc0, 0xc0, 0xc0);
static constexpr Color COLOR_DARK_GRAY = Color(0x40, 0x40, 0x40);
static constexpr Color COLOR_VERY_DARK_GRAY = Color(0x20, 0x20, 0x20);
static constexpr Color COLOR_RED = Color(0xff, 0x00, 0x00);
static constexpr Color COLOR_GREEN = Color(0x00, 0xff, 0x00);
static constexpr Color COLOR_BLUE = Color(0x00, 0x00, 0xff);
static constexpr Color COLOR_YELLOW = Color(0xff, 0xff, 0x00);
static constexpr Color COLOR_CYAN = Color(0x00, 0xff, 0xff);
static constexpr Color COLOR_MAGENTA = Color(0xff, 0x00, 0xff);
static constexpr Color COLOR_ORANGE = Color(0xff, 0xa2, 0x00);
static constexpr Color COLOR_BROWN = Color(0xb7, 0x64, 0x1e);

static constexpr Color COLOR_INVERSE_RED = Color(0xff, 0x70, 0x70);
static constexpr Color COLOR_INVERSE_BLUE = Color(0x90, 0x90, 0xff);
static constexpr Color COLOR_INVERSE_YELLOW = COLOR_YELLOW;
static constexpr Color COLOR_INVERSE_GREEN = COLOR_GREEN;
static constexpr Color COLOR_INVERSE_MAGENTA = COLOR_MAGENTA;

constexpr uint8_t
LightColor(uint8_t c) noexcept
{
  return ((c ^ 0xff) >> 1) ^ 0xff;
}

/**
 * Returns a lighter version of the specified color, adequate for
 * SRCAND filtering.
 */
constexpr Color
LightColor(Color c) noexcept
{
#ifdef GREYSCALE
  return Color(LightColor(c.GetLuminosity()));
#else
  return Color(LightColor(c.Red()), LightColor(c.Green()),
               LightColor(c.Blue()));
#endif
}

constexpr uint8_t
DarkColor(uint8_t c) noexcept
{
  return (c >> 1);
}

/**
 * Returns a darker version of the specified color.
 */
constexpr Color
DarkColor(Color c) noexcept
{
#ifdef GREYSCALE
  return Color(DarkColor(c.GetLuminosity()));
#else
  return Color(DarkColor(c.Red()), DarkColor(c.Green()),
               DarkColor(c.Blue()));
#endif
}

[[gnu::const]]
Color
Desaturate(Color c) noexcept;

constexpr Color
ColorWithAlpha(const Color &c, [[maybe_unused]] uint8_t a) noexcept
{
#ifdef ENABLE_OPENGL
  return c.WithAlpha(a);
#else
  return c;
#endif
}
