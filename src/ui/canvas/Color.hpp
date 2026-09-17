// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

// IWYU pragma: begin_exports
#ifdef ENABLE_OPENGL
#include "opengl/Color.hpp"
#elif defined(USE_MEMORY_CANVAS)
#include "memory/Color.hpp"
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

constexpr uint8_t
MixColors(uint8_t a, uint8_t b, uint8_t weight_a) noexcept
{
  return (uint8_t)((a * weight_a + b * (0xff - weight_a) + 0x7f) / 0xff);
}

/**
 * Blends two colors, like the CSS color-mix() function.
 *
 * @param weight_a the weight of color `a` (0..255, where 255 yields
 * `a` and 0 yields `b`)
 */
constexpr Color
MixColors(Color a, Color b, uint8_t weight_a) noexcept
{
#ifdef GREYSCALE
  return Color(MixColors(a.GetLuminosity(), b.GetLuminosity(), weight_a));
#else
  return Color(MixColors(a.Red(), b.Red(), weight_a),
               MixColors(a.Green(), b.Green(), weight_a),
               MixColors(a.Blue(), b.Blue(), weight_a));
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
