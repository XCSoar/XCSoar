/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_SCREEN_COLOR_HPP
#define XCSOAR_SCREEN_COLOR_HPP

// IWYU pragma: begin_exports
#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Color.hpp"
#elif defined(USE_MEMORY_CANVAS)
#include "Screen/Memory/Color.hpp"
#elif defined(USE_GDI)
#include "Screen/GDI/Color.hpp"
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

static inline constexpr uint8_t
LightColor(uint8_t c)
{
  return ((c ^ 0xff) >> 1) ^ 0xff;
}

/**
 * Returns a lighter version of the specified color, adequate for
 * SRCAND filtering.
 */
static inline constexpr Color
LightColor(Color c)
{
#ifdef GREYSCALE
  return Color(LightColor(c.GetLuminosity()));
#else
  return Color(LightColor(c.Red()), LightColor(c.Green()),
               LightColor(c.Blue()));
#endif
}

static inline constexpr uint8_t
DarkColor(uint8_t c)
{
  return (c >> 1);
}

/**
 * Returns a darker version of the specified color.
 */
static inline constexpr Color
DarkColor(Color c)
{
#ifdef GREYSCALE
  return Color(DarkColor(c.GetLuminosity()));
#else
  return Color(DarkColor(c.Red()), DarkColor(c.Green()),
               DarkColor(c.Blue()));
#endif
}

Color Desaturate(Color c);

static inline constexpr Color
ColorWithAlpha(const Color &c, uint8_t a)
{
#ifdef ENABLE_OPENGL
  return c.WithAlpha(a);
#else
  return c;
#endif
}

#endif
